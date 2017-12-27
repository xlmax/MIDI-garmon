/*
 MIDI note player (Когда-то это был проигрыватель нот MIDI, теперь это MIDI клавиатура)

 This sketch shows how to use the serial transmit pin (pin 1) to send MIDI note data.
 If this circuit is connected to a MIDI synth, it will play
 the notes F#-0 (0x1E) to F#-5 (0x5A) in sequence.


 The circuit:
 * digital in 1 connected to MIDI jack pin 5
 * MIDI jack pin 2 connected to ground
 * MIDI jack pin 4 connected to +5V through 220-ohm resistor
 Attach a MIDI cable to the jack, then to a MIDI synth, and play music.

 created 13 Jun 2006
 modified 13 Aug 2012
 by Tom Igoe
 modified 04 Nov 2016
 by Alexander Polikarpov

 This example code is in the public domain.
 
 http://www.arduino.cc/en/Tutorial/Midi

 */

// Раскомментируйте ЭТО если требуется отладка датчика давления. Для нормальной работы оставьте закомментированным.
//#define DEBUG_PRESSURE
 
#include <EEPROM.h>

// Подключите выводы Arduino Nano как указано ниже:

// Вывод Arduino "D1", номер контакта по схеме Ardiono Nano "J1-1" используется для MIDI выхода. Смотрите выше абзац "The circuit"
// Вывод Arduino "D0", номер контакта по схеме Ardiono Nano "J1-2" свободен. Можно использовать для любых целей, например для входа MIDI, светодиода или L10
// Вывод Arduino "A7", номер контакта по схеме Ardiono Nano "J2-5" свободен. Только аналоговый вход! Например, датчик давления.
// Вывод Arduino "A6", номер контакта по схеме Ardiono Nano "J2-6" свободен. Только аналоговый вход! Например, датчик давления.

// Шина данных, 8бит. Опрашиваем по 8 датчиков одновременно.
#define D0_PC0 0 // Вывод Arduino "A0", номер контакта по схеме Ardiono Nano "J2-12"
#define D1_PC1 1 // Вывод Arduino "A1", номер контакта по схеме Ardiono Nano "J2-11"
#define D2_PC2 2 // Вывод Arduino "A2", номер контакта по схеме Ardiono Nano "J2-10"
#define D3_PC3 3 // Вывод Arduino "A3", номер контакта по схеме Ardiono Nano "J2-9"
#define D4_PC4 4 // Вывод Arduino "A4", номер контакта по схеме Ardiono Nano "J2-8"
#define D5_PC5 5 // Вывод Arduino "A5", номер контакта по схеме Ardiono Nano "J2-7"
#define D6_PD6 6 // Вывод Arduino "D6", номер контакта по схеме Ardiono Nano "J1-9"
#define D7_PD7 7 // Вывод Arduino "D7", номер контакта по схеме Ardiono Nano "J1-10"

// Правая клавиатура (мелодия), выбор восьмёрки датчиков (56 кнопок, используется 52)
#define L0_PB2 2 // Вывод Arduino "D10", номер контакта по схеме Ardiono Nano "J1-13"
#define L1_PB4 4 // Вывод Arduino "D12", номер контакта по схеме Ardiono Nano "J1-15"
#define L2_PB3 3 // Вывод Arduino "D11", номер контакта по схеме Ardiono Nano "J1-14"
#define L3_PB0 0 // Вывод Arduino "D8" , номер контакта по схеме Ardiono Nano "J1-11"
#define L4_PB5 5 // Вывод Arduino "D13", номер контакта по схеме Ardiono Nano "J2-15"
#define L5_PD4 4 // Вывод Arduino "D4" , номер контакта по схеме Ardiono Nano "J1-7"
#define L6_PD5 5 // Вывод Arduino "D5" , номер контакта по схеме Ardiono Nano "J1-8"

// Левая кравиатура (басс-аккорд), выбор восьмёрки датчиков (24 кнопки, используется 24)
#define L7_PD2 2 // Вывод Arduino "D2" , номер контакта по схеме Ardiono Nano "J1-5"
#define L8_PD3 3 // Вывод Arduino "D3" , номер контакта по схеме Ardiono Nano "J1-6"
#define L9_PB1 1 // Вывод Arduino "D9" , номер контакта по схеме Ardiono Nano "J1-12"

// Контроллер, стоящий в Arduino Nano позволяет подключать указанные выше выводы почти в произвольном порядке.
// Важно выполнить некоторые условия:
// D1 всегда используется для MIDI, его нельзя использовать в матрице клавиатуры.
// Выводы с A0 по A5 могут использоваться только как D0 - D7 матрицы. Их нельзя использовать для L0 - L9. Их выходной ток недостаточен для питания датчиков Холла.
// Выводы D0, D2 - D13 можно использовать для любых целей.

// Определения датчика давления
#define PRESS_MAX_VALUE 25   // Значение с датчика выше которого будет выдаваться максимальная громкость. Максимально допустимое давление в мехе.
#define PRESS_MIN_VALUE 0    // Значение с датчика ниже которого команды MIDI не отправлются. Чтобы не отправлять команды при около-нулевой громкости.
#define PRESS_MAX_VOLUME 127 // Максимальное значение громкости, отправляемое в MIDI команде. 1..127
#define PRESS_FILTER 0.25    // Значение фильтра датчика давления. От 0 до 1. Ближе к 1 - слабая фильтрация. Ближе к 0 - сильная фильтрация.
// Глобальные переменные датчика давления
int press_raw_value;    // Выход с датчка необработанный
int press_out_value;    // Выход с датчика после обработки и ограничения
int press_prev_value;   // Предыдущее значение выхода датчика
int press_center_value; // Среднее значение выхода датчика, необходимо для нахождения абсолютного значения.

void setup() {
  // Инициализация последовательного порта и установка скорости MIDI (31250 бод):
#ifdef DEBUG_PRESSURE
  Serial.begin(57600);
#else
  Serial.begin(31250);
#endif  
  // Инициализация линий D0 - D7 на вход  
  DDRC  &= ~((1<<D0_PC0) | (1<<D1_PC1) | (1<<D2_PC2) | (1<<D3_PC3) | (1<<D4_PC4) | (1<<D5_PC5)); // Данные с холлов на вход
  DDRD  &= ~((1<<D6_PD6) | (1<<D7_PD7)); // Данные с холлов на вход
  PORTC &= ~((1<<D0_PC0) | (1<<D1_PC1) | (1<<D2_PC2) | (1<<D3_PC3) | (1<<D4_PC4) | (1<<D5_PC5)); // Установить нули
  PORTD &= ~((1<<D6_PD6) | (1<<D7_PD7)); // Установить нули
  // Инициализация линий L0 - L9 на выход
  DDRB  |=  ((1<<L9_PB1) | (1<<L0_PB2) | (1<<L2_PB3) | (1<<L1_PB4) | (1<<L3_PB0) | (1<<L4_PB5)); // На выход для управления холлами
  PORTB &= ~((1<<L9_PB1) | (1<<L0_PB2) | (1<<L2_PB3) | (1<<L1_PB4) | (1<<L3_PB0) | (1<<L4_PB5)); // На всех холлах нули
  DDRD   |=  ((1<<L7_PD2) | (1<<L8_PD3) | (1<<L5_PD4) | (1<<L6_PD5)); // На выход для управления холлами
  PORTD  &= ~((1<<L7_PD2) | (1<<L8_PD3) | (1<<L5_PD4) | (1<<L6_PD5)); // На всех холлах нули
  // Чтение среднего значения датчика давления   
  press_center_value = analogRead(6);
}

// Ниже определения нот в формате C=До D=Ре E=Ми F=Фа G=Соль A=Ля B=Си
// Строчная буква d=диез, b=бемоль. Цифра = номер октавы.
// Например Db2 = Ре бемоль 2й октавы
// Число после определения ноты - код данной ноты по стандарту MIDI

#define C2  0x18
#define Db2 0x19
#define D2  0x1A
#define Eb2 0x1B
#define E2  0x1C
#define F2  0x1D
#define Fd2 0x1E
#define G2  0x1F
#define Ab2 0x20
#define A2  0x21
#define Bb2 0x22
#define B2  0x23
#define C3  0x24
#define Db3 0x25
#define D3  0x26
#define Eb3 0x27
#define E3  0x28
#define F3  0x29
#define Fd3 0x2A
#define G3  0x2B
#define Ab3 0x2C
#define A3  0x2D
#define Bb3 0x2E
#define B3  0x2F
#define C4  0x30
#define Db4 0x31
#define D4  0x32
#define Eb4 0x33
#define E4  0x34
#define F4  0x35
#define Fd4 0x36
#define G4  0x37
#define Ab4 0x38
#define A4  0x39
#define Bb4 0x3A
#define B4  0x3B
#define C5  0x3C
#define Db5 0x3D
#define D5  0x3E
#define Eb5 0x3F
#define E5  0x40
#define F5  0x41
#define Fd5 0x42
#define G5  0x43
#define Ab5 0x44
#define A5  0x45
#define Bb5 0x46
#define B5  0x47
#define C6  0x48
#define Db6 0x49
#define D6  0x4A
#define Eb6 0x4B
#define E6  0x4C
#define F6  0x4D
#define Fd6 0x4E
#define G6  0x4F
#define Ab6 0x50
#define A6  0x51
#define Bb6 0x52
#define B6  0x53
#define C7  0x54
#define Db7 0x55
#define D7  0x56
#define Eb7 0x57
#define E7  0x58
#define F7  0x59
#define Fd7 0x5A
#define G7  0x5B
#define Ab7 0x5C
#define A7  0x5D
#define Bb7 0x5E
#define B7  0x5F
#define C8  0x60
#define Db8 0x61
#define D8  0x62
#define Eb8 0x63
#define E8  0x64
#define F8  0x65
#define Fd8 0x66
#define G8  0x67
#define Ab8 0x68
#define A8  0x69
#define Bb8 0x6A
#define B8  0x6B
#define C9  0x6C
#define Db9 0x6D
#define D9  0x6E
#define Eb9 0x6F
#define E9  0x70
#define F9  0x71
#define Fd9 0x72
#define G9  0x73
#define Ab9 0x74
#define A9  0x75
#define Bb9 0x76
#define B9  0x77

#define NNN (0) // Отсутствие ноты

// Временные переменные для чтения клавиатуры
char datap[10] = {0,0,0,0,0,0,0,0,0,0}; // Предыдущее значение нажатых кнопок
char data[10]  = {0,0,0,0,0,0,0,0,0,0}; // Текущее считанное значение нажатых кнопок

char notes[10][8]  = {
// Для Вашего инструмента нужно будет правильно заполнить эту матрицу.
// Соответствие клавиш нотам. Каждая строка соответствует линиям:
// D0, D1, D2, D3, D4, D5, D6 ,D7
  {Ab6,E6 ,Db6,A5 ,Fd5,D5 ,B4 ,Ab4}, // Для L0 Дальний правый ряд, снизу вверх
  {E4 ,Db4,A3 ,F4 ,C4 				 // Для L1 
					  ,A6 ,Fd6,D6 }, // Для L1 Ближний правый ряд, снизу вверх
  {B5 ,Ab5,E5 ,Db5,A4 ,Fd4,D4 ,B3 }, // Для L2 
  {Eb4,NNN,NNN,NNN,NNN,NNN,NNN,NNN}, // Для L3 
  {Ab2,A2 ,B2 ,Db3,D3 ,E3 ,Fd3,NNN}, // Для L4 Басы, сверху вниз
  {Ab3,A3 ,B3 ,C4 ,Db4,D4 ,Fd4,E4 }, // Для L5 Аккорды сверху вниз
  {F4 ,Fd4,NNN,NNN,NNN,NNN,NNN,NNN}, // Для L6
// Например, кнопка, стоящая на пересечении D2 и L4 соответствует Eb4 
// NNN означает отсутствие ноты, кнопка не используется для извлечения звуков
  };

// Наборы альтернативной раскладки нот (заполняйте как вам удобно)

char alt_notes_0[10][8][4] = {
// Соответствие клавиш нотам (одна клавиша - 4е ноты). Каждая строка соответствует линиям:
//  D0,                  D1,                  D2,                  D3,                  D4,                  D5,                  D6,                  D7
  //правая клавиатура-дальний ряд снизу вверх
  {{Ab6, Ab5, NNN, NNN},{E6 , E5 , NNN, NNN},{Db6, Db5, NNN, NNN},{A5 , A4 , NNN, NNN},{Fd5, Fd4, NNN, NNN},{D5 , D4 , NNN, NNN},{B4 , B3 , NNN, NNN},{Ab4, Ab3, NNN, NNN}}, // Для L0
																											//ближний правый ряд, снизу вверх
  {{E4 , E3 , NNN, NNN},{Db4, Db3, NNN, NNN},{A3 , A2 , NNN, NNN},{F4 , F3 , NNN, NNN},{C4 , C3 , NNN, NNN},{A6 , A5 , NNN, NNN},{Fd6, Fd5, NNN, NNN},{D6 , D5 , NNN, NNN}}, // Для L1
  {{B5 , B4 , NNN, NNN},{Ab5, Ab4, NNN, NNN},{E5 , E4 , NNN, NNN},{Db5, Db4, NNN, NNN},{A4 , A5 , NNN, NNN},{Fd4, Fd3, NNN, NNN},{D4 , D3 , NNN, NNN},{B3 , B2 , NNN, NNN}}, // Для L2
  {{Eb4, Eb3, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN}}, // Для L3
  //басы, сверху вниз
  {{Ab2, NNN, NNN, NNN},{A2 , NNN, NNN, NNN},{B2 , NNN, NNN, NNN},{Db3, NNN, NNN, NNN},{D3 , NNN, NNN, NNN},{E3 , NNN, NNN, NNN},{Fd3, NNN, NNN, NNN},{NNN, NNN, NNN, NNN}}, // Для L4
  //Аккорды сверху вниз
  {{Ab3, NNN, NNN, NNN},{A3 , NNN, NNN, NNN},{B3 , NNN, NNN, NNN},{C4 , NNN, NNN, NNN},{Db4, NNN, NNN, NNN},{D4 , NNN, NNN, NNN},{Fd4, NNN, NNN, NNN},{E4 , NNN, NNN, NNN}}, // Для L5
  {{F4 , NNN, NNN, NNN},{Fd4, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN}}, // Для L6
  // NNN означает отсутствие ноты,
};

char alt_notes_1[10][8][4] = {
// Соответствие клавиш нотам (одна клавиша - 4е ноты). Каждая строка соответствует линиям:
//  D0,                  D1,                  D2,                  D3,                  D4,                  D5,                  D6,                  D7
  //правая клавиатура-дальний ряд снизу вверх
  {{Ab6, Ab5, NNN, NNN},{E6 , E5 , NNN, NNN},{Db6, Db5, NNN, NNN},{A5 , A4 , NNN, NNN},{Fd5, Fd4, NNN, NNN},{D5 , D4 , NNN, NNN},{B4 , B3 , NNN, NNN},{Ab4, Ab3, NNN, NNN}}, // Для L0
																											//ближний правый ряд, снизу вверх
  {{E4 , E3 , NNN, NNN},{Db4, Db3, NNN, NNN},{A3 , A2 , NNN, NNN},{F4 , F3 , NNN, NNN},{C4 , C3 , NNN, NNN},{A6 , A5 , NNN, NNN},{Fd6, Fd5, NNN, NNN},{D6 , D5 , NNN, NNN}}, // Для L1
  {{B5 , B4 , NNN, NNN},{Ab5, Ab4, NNN, NNN},{E5 , E4 , NNN, NNN},{Db5, Db4, NNN, NNN},{A4 , A5 , NNN, NNN},{Fd4, Fd3, NNN, NNN},{D4 , D3 , NNN, NNN},{B3 , B2 , NNN, NNN}}, // Для L2
  {{Eb4, Eb3, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN}}, // Для L3
  //басы, сверху вниз
  {{Ab2, Ab3, NNN, NNN},{A2 , A3 , NNN, NNN},{B2 , B3 , NNN, NNN},{Db2, Db3, NNN, NNN},{D2 , D3 , NNN, NNN},{E2 , E3 , NNN, NNN},{Fd2, Fd3, NNN, NNN},{NNN, NNN, NNN, NNN}}, // Для L4
  //Аккорды сверху вниз
  {{Ab3, Ab4, NNN, NNN},{A3 , A4 , NNN, NNN},{B3 , B4 , NNN, NNN},{C3 , C4 , NNN, NNN},{Db3, Db4, NNN, NNN},{D3 , D4 , NNN, NNN},{Fd3, Fd4, NNN, NNN},{E3 , E4 , NNN, NNN}}, // Для L5
  {{F3 , F4 , NNN, NNN},{Fd3, Fd4, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN},{NNN, NNN, NNN, NNN}}, // Для L6
  // NNN означает отсутствие ноты,
};



// Дополнительные функции клавиш, их 16:
#define ___ 0x00 // - не назначена дополнительная функция
#define _MD 0x01 // - кнопка Режим - если она нажата, разрешены дополнительные функции клавиш
#define _PI 0x02 // - кнопка Плюс инструмент - выбрать следующий инструмент для текущего канала
#define _MI 0x03 // - кнопка Минус инструмент - выбрать предыдущий инструмент для текущего канала
#define _PV 0x04 // - кнопка Плюс громкость - увеличить громкость для текущего канала
#define _MV 0x05 // - кнопка Минус громкость - уменьшить громкость для текущего канала
#define _C0 0x06 // - кнопка Канал 0 - выбор канала 0
#define _C1 0x07 // - кнопка Канал 1 - выбор канала 1
#define _C2 0x08 // - кнопка Канал 2 - выбор канала 2
#define _C3 0x09 // - кнопка Канал 3 - выбор канала 3
#define _C4 0x0A // - кнопка Канал 4 - выбор канала 4
#define _C5 0x0B // - кнопка Канал 5 - выбор канала 5
#define _R0 0x0C // - регистр 0, сохранённая конфигурация инструментов
#define _R1 0x0D // - регистр 1, сохранённая конфигурация инструментов
#define _R2 0x0E // - регистр 2, сохранённая конфигурация инструментов
#define _R3 0x0F // - регистр 3, сохранённая конфигурация инструментов
#define _R4 0x10 // - регистр 4, сохранённая конфигурация инструментов
#define _R5 0x11 // - регистр 5, сохранённая конфигурация инструментов
#define _R6 0x12 // - регистр 6, сохранённая конфигурация инструментов
#define _R7 0x13 // - регистр 7, сохранённая конфигурация инструментов
#define _EP 0x14 // - кнопка эффекта Pitch (Опытная функция)
#define _PS 0x15 // - кнопка датчика давления. Одно нажатие отключает, второе включает.
#define _PB 0x16 // - кнопка Плюс банк - полседовательно переключает банки.
#define _LD 0x17 // - кнопка Блокировка ударных
#define _LO 0x18 // - кнопка смены "раскладки" нот

char func_buttons[16]; // массив состояния дополнительных функций

char func[10][8]  = {
// Для Вашего инструмента нужно будет правильно заполнить эту матрицу.
// Соответствие клавиш дополнительным функциям. Каждая строка соответствует линиям:
// D0, D1, D2, D3, D4, D5, D6 ,D7
  {_MD,_C0,_C3,_C1,_C4,_C2,_C5,___}, // Для L0
  {___,___,___,___,___,_PI,_MI,_PV}, // Для L1
  {_MV,_LO,_LD,_PB,_PS,_EP,___,___}, // Для L2
  {___,___,___,___,___,___,___,___}, // Для L3
  {_R0,_R1,_R2,_R3,_R4,_R5,_R6,___}, // Для L4
  {___,___,___,___,___,___,___,___}, // Для L5
  {___,___,___,___,___,___,___,___}, // Для L6
  {___,___,___,___,___,___,___,___}, // Для L7
  {___,___,___,___,___,___,___,___}, // Для L8
  {___,___,___,___,___,___,___,___}, // Для L9
// Например, кнопка, стоящая на пересечении D3 и L6 соответствует _MD, кнопке "Режим" 
  };

char chan[10][8]  = {
// Для Вашего инструмента нужно будет правильно заполнить эту матрицу.
// Соответствие клавиш MIDI каналам. 
// Числа, которыми заполнена матрица - шестнадцатиричные. Это удобно для назначения кнопки сразу на два канала.
// Например:
// 0x00 означает что нота звучит только на канале 0
// 0x11 означает что нота звучит только на канале 1
// 0xFF означает что нота звучит только на канале 15
// 0xF1 означает что нота звучит на канале 15 и на канале 1
// 0x18 означает что нота звучит на канале 1 и на канале 8
// Можно назначить ноту на любой из каналов, но он не должен быть больше чем MD_MAX_CHANNEL
// Каждая строка соответствует линиям:
// D0,  D1,  D2,  D3,  D4,  D5,  D6 , D7
  {0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03}, // Для L0
  {0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03}, // Для L1
  {0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03}, // Для L2
  {0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03}, // Для L3
  {0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14}, // Для L4
  {0x25,0x25,0x25,0x25,0x25,0x25,0x25,0x25}, // Для L5
  {0x25,0x25,0x25,0x25,0x25,0x25,0x25,0x25}, // Для L6
  };

// Эти каналы должны быть такими же как и в таблице выше. Их можно переназначить.
#define CHANNEL0 0
#define CHANNEL1 1
#define CHANNEL2 2
#define CHANNEL3 3
#define CHANNEL4 4
#define CHANNEL5 5

// Макрос подмены канала 4-го на 9-й
#define _CH(X) (((X) == 4) ? 9 : (X))

#define MD_MAX_INSTRUMENT 127
#define MD_MAX_VOLUME 127
#define MD_MAX_CHANNEL 6
#define MD_SIZE ((MD_MAX_CHANNEL*3) + 1)
#define MD_SAVE_DELAY 4000 // Задержка в миллисекундах до сохранения настроек
#define MD_DEFAULT_NOTE C5
#define MD_EFFECT_NO 0
#define MD_EFFECT_PITCH 1
#define MD_EFFECT_CHORUS 2
#define MD_EFFECT_REVERB 3

typedef struct {
  char banks[5];
  char banks_count;
  char has_drums_banks;
} dev_t; 

typedef enum {dev_china, dev_yamaha, dev_ketron, dev_count} devs_ids_t;  // Типы подключаемых устройств

const dev_t devs[dev_count] = {
  {{0,  0,  0,   0,   0}, 1, 0},  // dev_china, только 1н 0-й банк, банки на ударных менять нельзя
  {{0, 48, 64, 126, 128}, 5, 0},  // dev_yamaha, 5 банков, банки на ударных менять нельзя(0)
  {{0,  1,  10,  2,   4}, 5, 1},  // dev_ketron, 5 банков, банки на ударных менять МОЖНО(1)
};

dev_t* pdev = &devs[dev_yamaha];  // Будем работять с yamaha (нужно выбрать нужное устройство)

// Структура, используемая для сохранения настроек
typedef struct {
  char md_channel_instrument[MD_MAX_CHANNEL];
  char md_channel_volume[MD_MAX_CHANNEL];
  char md_channel_bank[MD_MAX_CHANNEL];
  char md_pitch[MD_MAX_CHANNEL];
  struct {
    unsigned char md_pressure : 1;
    unsigned char md_lock_drums : 1;
    unsigned char md_layout : 2;
    unsigned char _ : 4;
  } option;

} MD_DATA;

// Глобальные переменные, используются для изменения текущго режима
char md_pressure_lock = 0;
char md_active = 0;
char c0_active = 0;
char c1_active = 0;
char c2_active = 0;
char c3_active = 0;
char c4_active = 0;
char c5_active = 0;
char md_current_channel = 0;
char md_current_effect = MD_EFFECT_NO;
long int md_delay = 0;
long int md_delay_ok = 0;
long int md_offset = 0;
//char md_chan_h = 0;
//char md_chan_l = 0;
MD_DATA md_data = {{0,0,0,0,0,0},{64,64,64,64,64,64},{0,0,0,0,0,0},{64,64,64,64,64,64},0};
char* p_md_data = (char*)&md_data;

char i = 0;
char j = 0;

bool lock_by_drums(char ch) {
  return md_data.option.md_lock_drums && (ch == CHANNEL4);
}

void play_key(char key_i, char key_j, char state) {
  char note;
  char chanels[2] = {(chan[key_i][key_j] >> 4) & 0x0F,
                      chan[key_i][key_j] & 0x0F};
                      
  char ch_cnt = (chanels[0] == chanels[1]) ? 1 : 2;

  if (md_active) return;

  if (md_data.option.md_layout) {  // У нас альтернативная раскладка
    for (char idx = 0; idx < 4; idx++) {
      note = (md_data.option.md_layout == 1) ? alt_notes_0[key_i][key_j][idx]
                                             : alt_notes_1[key_i][key_j][idx];

      if (note != NNN) {
        for (char ch_idx = 0; ch_idx < ch_cnt; ch_idx++) {
          bool can_play = !lock_by_drums(chanels[ch_idx]) &&
                          md_data.md_channel_volume[chanels[ch_idx]];

          if (state) {
            if (can_play) noteOn(0x90, chanels[ch_idx], note, 127);          
          } else {
            if (can_play) noteOn(0x80, chanels[ch_idx], note, 0);  
          }        
        }
      }
    }
  } else {  // Раскладка по умолчанию
    note = notes[key_i][key_j];

    if (note != NNN) {
      for (char ch_idx = 0; ch_idx < ch_cnt; ch_idx++) {
        bool can_play = !lock_by_drums(chanels[ch_idx]) &&
                        md_data.md_channel_volume[chanels[ch_idx]];

        if (state) {
          if (can_play) noteOn(0x90, chanels[ch_idx], note, 127);          
        } else {
          if (can_play) noteOn(0x80, chanels[ch_idx], note, 0);  
        }        
      }
    }
  }
}

// В цикле происходит опрос клавиш и отправка MIDI команд
void loop() {
  // ОБРАБОТКА ДАТЧИКА ДАВЛЕНИЯ
  // Считать сырое (необработанное) значение датчика давления и вычесть из него среднее значение
  press_raw_value = analogRead(6) - press_center_value;
  // Значение должно быть положительным. А также отфильтровать его.
  press_out_value = PRESS_FILTER*((float)(abs(press_raw_value))) + ((float)1.0-PRESS_FILTER)*(float)press_out_value;
  // "Мёртвая зона" ниже которой выход датчика считается нулём.
  if (press_out_value < PRESS_MIN_VALUE) press_out_value = 0; else press_out_value -= PRESS_MIN_VALUE;
  // Ограничить значение. Оно должно быть не более максимального.
  if (press_out_value > PRESS_MAX_VALUE) press_out_value = PRESS_MAX_VALUE;
  // Если значение датчика поменялось, послать команду изменения громкости.
  if (press_out_value != press_prev_value)
  {
#ifdef DEBUG_PRESSURE
    // Вывести в порт значнеие с датчика 
    Serial.println(press_out_value ,DEC);
#else 
    // Для всех каналов   
    for (int channel=0; channel<MD_MAX_CHANNEL; channel++)
    {
      // Вычислить громкость канала из заданной громкости и выхода датчика давления.
      // В зависимости от выхода датчика давления громкость будет изменяться от нуля до текущей заданной громкости канала.
      int volume = (((long int)(md_data.md_channel_volume[channel]) * press_out_value / PRESS_MAX_VALUE));
      // 0xB0 MIDI_CONTROL_CHANGE
      // 0x07 MIDI_CONTROL_CHANNEL_VOLUME
      if ((md_data.option.md_pressure == 1) && (md_pressure_lock == 0)) // Если датчик включен и не заблокирован кнопкой "Режим"
        Command3(0xB0, channel, 0x07,volume);
    }
#endif    
  }
  press_prev_value = press_out_value; // ЭТУ строку вставить
  // ОБРАБОТКА КНОПОК
  data[i] = 0;
  // Включить линию датчиков Холла в соответствии со значением счётчика.
  // С каждым циклом выбираем одну из линий.
  // Если нужно добавить ещё линию, то добавьте ещё одну строку ниже:
  if (0 == i) PORTB |= (1<<L0_PB2); // Включен L0
  if (1 == i) PORTB |= (1<<L1_PB4); // Включен L1
  if (2 == i) PORTB |= (1<<L2_PB3); // Включен L2
  if (3 == i) PORTB |= (1<<L3_PB0); // Включен L3
  if (4 == i) PORTB |= (1<<L4_PB5); // Включен L4
  if (5 == i) PORTD |= (1<<L5_PD4); // Включен L5
  if (6 == i) PORTD |= (1<<L6_PD5); // Включен L6
  if (7 == i) PORTD |= (1<<L7_PD2); // Включен L7
  if (8 == i) PORTD |= (1<<L8_PD3); // Включен L8
  if (9 == i) PORTB |= (1<<L9_PB1); // Включен L9

  // Задержка на 2000 счётов
  for (volatile int a=0;a<2500;a++);

  // Собрать один байт данных из линий D0 - D7
  data[i] |= PINC & ((1<<D0_PC0) | (1<<D1_PC1) | (1<<D2_PC2) | (1<<D3_PC3) | (1<<D4_PC4) | (1<<D5_PC5));
  data[i] |= PIND & ((1<<D6_PD6) | (1<<D7_PD7));

  // Если предыдущее состояние кнопок не равно текущему. То есть что-то стало нажато или отпущено.
  //if (datap[i] != data[i])
  {
    // Перебрать все 8 бит данных data
    for (j=0; j<8; j++)  
    {
      // Если кнопка была отжата
      if (( data[i] & (1<<j)) && (~datap[i] & (1<<j)))
      {
        // Взять из таблицы дополнительную функцию кнопки
        char current_func = func[i][j];
        // Если функция кнопки - "Режим", то разрешить дополнительные функции кнопок и запретить звучание
        if ((current_func == _MD)) {
          md_active = 0;
          md_pressure_lock = 0; // Разблокировать датчик давления 
          for (int cnt=0;cnt<MD_MAX_CHANNEL;cnt++) // На всех каналах
          {
            Command3 (0xB0,cnt,0x7B,0);        // Отключить звучание
            Command3 (0xB0,cnt,0x07,md_data.md_channel_volume[cnt]); // задать громкость всем каналам
          }  
        }

        // Если функция кнопки - "Канал 0" .. "Канал 5", то выставить флаг нажатия        
        if ((current_func == _C0)) c0_active = 0;
        if ((current_func == _C1)) c1_active = 0;
        if ((current_func == _C2)) c2_active = 0;
        if ((current_func == _C3)) c3_active = 0;
        if ((current_func == _C4)) c4_active = 0;
        if ((current_func == _C5)) c5_active = 0;

        // Дополнительные функции обрабатываются только при нажатой кнопке "Режим"
        if (md_active == 1)
        {
          char register_changed = 0;
          if (current_func == _R0) // Отжата кнопка "Регистр 0", прочитать регистр 0
          {
            md_offset = 0;
            register_changed = 1;
          }
          if (current_func == _R1) // Отжата кнопка "Регистр 1", прочитать регистр 1
          {
            md_offset = MD_SIZE*1;
            register_changed = 1;
          }
          if (current_func == _R2) // Отжата кнопка "Регистр 2", прочитать регистр 2
          {
            md_offset = MD_SIZE*2;
            register_changed = 1;
          }
          if (current_func == _R3) // Отжата кнопка "Регистр 3", прочитать регистр 3
          {
            md_offset = MD_SIZE*3;
            register_changed = 1;
          }
          if (current_func == _R4) // Отжата кнопка "Регистр 4", прочитать регистр 4
          {
            md_offset = MD_SIZE*4;
            register_changed = 1;
          }
          if (current_func == _R5) // Отжата кнопка "Регистр 5", прочитать регистр 5
          {
            md_offset = MD_SIZE*5;
            register_changed = 1;
          }
          if (current_func == _R6) // Отжата кнопка "Регистр 6", прочитать регистр 6
          {
            md_offset = MD_SIZE*6;
            register_changed = 1;
          }
          if (current_func == _R7) // Отжата кнопка "Регистр 7", прочитать регистр 7
          {
            md_offset = MD_SIZE*7;
            register_changed = 1;
          }
          // Отжата одна из кнопок плюс/минус
          if ((current_func == _PI) || (current_func == _MI) || (current_func == _PV) || (current_func == _MV) || 
              (current_func == _C0) || (current_func == _C1) || (current_func == _C2) ||
              (current_func == _C3) || (current_func == _C4) || (current_func == _C5) || (current_func == _EP) ||
              (current_func == _PB))
          {
            for (int cnt=0;cnt<MD_MAX_CHANNEL;cnt++) // На всех каналах
              Command3 (0xB0,cnt,0x7B,0); // Отключить звучание
          }

          // Применить изменения, если они были
          if (register_changed == 1)
          {
            // Отменить таймер сохранения
            md_delay_ok = md_delay;
            // Прочитать конфигурацию
            for (int cnt = 0; cnt<MD_MAX_CHANNEL;   cnt++)
            { 
              md_data.md_channel_instrument[cnt] = EEPROM.read(cnt+md_offset);
            }
            for (int cnt = 0; cnt<MD_MAX_CHANNEL; cnt++) 
            {
              md_data.md_channel_volume[cnt] = EEPROM.read(MD_MAX_CHANNEL+cnt+md_offset);
            }
            for (int cnt = 0; cnt<MD_MAX_CHANNEL; cnt++)
            {
              md_data.md_channel_bank[cnt] = EEPROM.read((MD_MAX_CHANNEL*2)+cnt+md_offset);
            }
            *((char*) &md_data.option) = EEPROM.read((MD_MAX_CHANNEL*3)+md_offset);
            
            // Для всех каналов задать инструмент и громкость
            for (int cnt =0; cnt<MD_MAX_CHANNEL; cnt++)
            {
              // Если после чтения значения вышли за пределы - исправить
              if (md_data.md_channel_bank[cnt] >= pdev->banks_count) md_data.md_channel_bank[cnt] = 0;
              if (md_data.md_channel_instrument[cnt] > MD_MAX_INSTRUMENT) md_data.md_channel_instrument[cnt] = 0;
              if (md_data.md_channel_volume[cnt] > MD_MAX_VOLUME) md_data.md_channel_volume[cnt] = 64;

              // Задать параметры
              if (!(cnt == CHANNEL4 && pdev->has_drums_banks == 0)) {  // Можно сменить банк 
                Command3 (0xB0,cnt,0x00,pdev->banks[md_data.md_channel_bank[cnt]]);
                for (char a=0;a<100;a++);
              }
              Command2 (0xC0,cnt,md_data.md_channel_instrument[cnt]);
              for (char a=0;a<100;a++);
              Command3 (0xB0,cnt,0x07,md_data.md_channel_volume[cnt]);
              for (char a=0;a<100;a++);
            }
          }
        }
        play_key(i, j, 0x00);
      }
      // Если кнопка была нажата
      if ((~data[i] & (1<<j)) && ( datap[i] & (1<<j)))
      {
        // Взять из таблицы дополнительную функцию кнопки
        char current_func = func[i][j];
        // Если функция кнопки - "Режим", то разрешить дополнительные функции кнопок и прекратить звучание
        if ((current_func == _MD)){
          md_active = 1;                           // Разрешить дополнительные функции
          md_pressure_lock = 1;                    // Заблокировать датчик давления 
          for (int cnt=0;cnt<MD_MAX_CHANNEL;cnt++) // На всех каналах
          {
            Command3 (0xB0,cnt,0x7B,0);        // Отключить звучание
            Command3 (0xB0,cnt,0x07,md_data.md_channel_volume[cnt]); // задать громкость всем каналам
          }  
        }
        // Дополнительные функции обрабатываются только при нажатой кнопке "Режим"
        if (md_active == 1)
        {        
          char register_changed = 0;

          if (current_func == _PS)  // Нажата кнопка - "Датчик давления" 
          {
            if (md_data.option.md_pressure == 1) // если был включен, то выключтить
              md_data.option.md_pressure = 0;
            else                          // если был выключен, то включить
            {
              press_center_value = analogRead(6);
              md_data.option.md_pressure = 1;
              for (int cnt=0;cnt<MD_MAX_CHANNEL;cnt++) // На всех каналах
                Command3 (0xB0,cnt,0x07,0); // задать нулевую громкость всем каналам
            }
          }

          if (current_func == _LD)  // Нажата кнопка - "Блокировка ударных"
          {
            md_data.option.md_lock_drums = md_data.option.md_lock_drums ? 0 : 1;
            register_changed = 1;
          }

          // Нажата кнопка "Канал 0", выбрать канал 0
          if (current_func == _C0) {md_current_channel = CHANNEL0; c0_active = 1;md_current_effect = MD_EFFECT_NO;
            Command2 (0xC0,md_current_channel,md_data.md_channel_instrument[md_current_channel]);
            noteOn(0x90,md_current_channel, MD_DEFAULT_NOTE, 127);}
          // Нажата кнопка "Канал 1", выбрать канал 1
          if (current_func == _C1) {md_current_channel = CHANNEL1; c1_active = 1;md_current_effect = MD_EFFECT_NO;
            Command2 (0xC0,md_current_channel,md_data.md_channel_instrument[md_current_channel]);
            noteOn(0x90,md_current_channel, MD_DEFAULT_NOTE, 127);}
          // Нажата кнопка "Канал 2", выбрать канал 2
          if (current_func == _C2) {md_current_channel = CHANNEL2; c2_active = 1;md_current_effect = MD_EFFECT_NO;
            Command2 (0xC0,md_current_channel,md_data.md_channel_instrument[md_current_channel]);
            noteOn(0x90,md_current_channel, MD_DEFAULT_NOTE, 127);}
          // Нажата кнопка "Канал 3", выбрать канал 3
          if (current_func == _C3) {md_current_channel = CHANNEL3; c3_active = 1;md_current_effect = MD_EFFECT_NO;
            Command2 (0xC0,md_current_channel,md_data.md_channel_instrument[md_current_channel]);
            noteOn(0x90,md_current_channel, MD_DEFAULT_NOTE, 127);}
          // Нажата кнопка "Канал 4", выбрать канал 4 (9 ударные)
          if (current_func == _C4) {md_current_channel = CHANNEL4; c4_active = 1;md_current_effect = MD_EFFECT_NO;
            Command2 (0xC0,md_current_channel,md_data.md_channel_instrument[md_current_channel]);
            noteOn(0x90,md_current_channel, MD_DEFAULT_NOTE, 127);}
          // Нажата кнопка "Канал 5", выбрать канал 5
          if (current_func == _C5) {md_current_channel = CHANNEL5; c5_active = 1;md_current_effect = MD_EFFECT_NO;
            Command2 (0xC0,md_current_channel,md_data.md_channel_instrument[md_current_channel]);
            noteOn(0x90,md_current_channel, MD_DEFAULT_NOTE, 127);}
          // Нажата кнопка "Эффект Pitch"
          if (current_func == _EP) {md_current_effect = MD_EFFECT_PITCH;
            Command3 (0xE0,md_current_channel, 0,md_data.md_pitch[md_current_channel]);
            noteOn(0x90,md_current_channel, MD_DEFAULT_NOTE, 127);}
          // Нажата кнопка "Плюс", задать следующий инструмент и подать команду о его смене для текущего канала
          if (current_func == _PI)
          {
            if (md_current_effect == MD_EFFECT_NO)
            {
              if (md_data.md_channel_instrument[md_current_channel]<MD_MAX_INSTRUMENT) // Проверка максимального значения
              {
                md_data.md_channel_instrument[md_current_channel]++; // Прибавить 1 к инструменту
                Command2 (0xC0,md_current_channel,md_data.md_channel_instrument[md_current_channel]); // Сменить инструмент
              }
            }
            if (md_current_effect == MD_EFFECT_PITCH)
            {
              if (md_data.md_pitch[md_current_channel]<MD_MAX_INSTRUMENT) // Проверка максимального значения
              {
                md_data.md_pitch[md_current_channel]++; // Прибавить 1
                Command3 (0xE0,md_current_channel, 0,md_data.md_pitch[md_current_channel]); // Сменить pitch
              }
            }
            noteOn(0x90,md_current_channel, MD_DEFAULT_NOTE, 127); // Начать звучание
          }
          // Нажата кнопка "Минус", задать предыдущий инструмент и подать команду о его смене для текущего канала
          if (current_func == _MI)
          {
            if (md_current_effect == MD_EFFECT_NO)
            {
              if (md_data.md_channel_instrument[md_current_channel]>0) // Проверка минимального значения
              {
                md_data.md_channel_instrument[md_current_channel]--; // Вычесть 1 из инструмента
                Command2 (0xC0,md_current_channel,md_data.md_channel_instrument[md_current_channel]); // Сменить
              }
            }
            if (md_current_effect == MD_EFFECT_PITCH)
            {
              if (md_data.md_pitch[md_current_channel]>0) // Проверка минимального значения
              {
                md_data.md_pitch[md_current_channel]--; // Вычесть 1
                Command3 (0xE0,md_current_channel, 0,md_data.md_pitch[md_current_channel]); // Сменить pitch
              }
            }
            noteOn(0x90,md_current_channel, MD_DEFAULT_NOTE, 127); // Начать звучание установленным инструментом
          }
          // Нажата кнопка "Плюс громкость", увеличить громкость на 1 и подать команду о смене громкости для текущего канала
          if (current_func == _PV)
          {
            if (md_data.md_channel_volume[md_current_channel]<MD_MAX_VOLUME) // Проверка максимального значения
            {
              md_data.md_channel_volume[md_current_channel]++; // Прибавить 1 к громкости
              Command3 (0xB0,md_current_channel,0x07,md_data.md_channel_volume[md_current_channel]); // Сменить громкость
            }
            noteOn(0x90,md_current_channel, MD_DEFAULT_NOTE, 127); // Начать звучание установленным инструментом
          }
          // Нажата кнопка "Минус громкость", уменьшить громкость на 1 и подать команду о смене громкости для текущего канала
          if (current_func == _MV)
          {
            if (md_data.md_channel_volume[md_current_channel]>0) // Проверка минимального значения
            {
              md_data.md_channel_volume[md_current_channel]--; // Убавить 1 из громкости
              Command3 (0xB0,md_current_channel,0x07,md_data.md_channel_volume[md_current_channel]); // Сменить громкость
            }
            noteOn(0x90,md_current_channel, MD_DEFAULT_NOTE, 127); // Начать звучание установленным инструментом
          }
          // Нажата кнопка "Плюс банк", сменить банк и подать команду о смене банка для текущего канала (если разрешено)
          if (current_func == _PB && pdev->banks_count && !lock_by_drums(md_current_channel) && pdev->has_drums_banks)
          {
            if (++md_data.md_channel_bank[md_current_channel] >= pdev->banks_count) {
              md_data.md_channel_bank[md_current_channel] = 0;
            }

            Command3 (0xB0,md_current_channel,0x00,pdev->banks[md_data.md_channel_bank[md_current_channel]]); // Сменить банк
            noteOn(0x90,md_current_channel, MD_DEFAULT_NOTE, 127); // Начать звучание установленным инструментом
          }

          // Нажата кнопка "смена раскладки нот"
          if (current_func == _LO)
          {
            if (md_data.option.md_layout < 2) {
              md_data.option.md_layout++;
            } else {
              md_data.option.md_layout = 0;
            }

            noteOn(0x90,md_current_channel, MD_DEFAULT_NOTE, 127); // Начать звучание установленным инструментом
          }          
          
          // Нажата одна из кнопок регистров. Запомнить номер регистра.
          if (current_func == _R0) // Нажата кнопка "Регистр 0"
            { md_offset = MD_SIZE*0;register_changed = 1;}
          if (current_func == _R1) // Нажата кнопка "Регистр 1"
            { md_offset = MD_SIZE*1;register_changed = 1;}
          if (current_func == _R2) // Нажата кнопка "Регистр 2"
            { md_offset = MD_SIZE*2;register_changed = 1;}
          if (current_func == _R3) // Нажата кнопка "Регистр 3"
            { md_offset = MD_SIZE*3;register_changed = 1;}
          if (current_func == _R4) // Нажата кнопка "Регистр 4"
            { md_offset = MD_SIZE*4;register_changed = 1;}
          if (current_func == _R5) // Нажата кнопка "Регистр 5"
            { md_offset = MD_SIZE*5;register_changed = 1;}
          if (current_func == _R6) // Нажата кнопка "Регистр 6"
            { md_offset = MD_SIZE*6;register_changed = 1;}
          if (current_func == _R7) // Нажата кнопка "Регистр 7"
            { md_offset = MD_SIZE*7;register_changed = 1;}

          // Сохранить изменения, если они были, а также если нажаты все три кнопки канала одновременно
          if ((register_changed == 1) && (c0_active == 1) && (c1_active == 1) && (c2_active == 1))
          {
            // Записать
            for (int cnt = 0; cnt<MD_MAX_CHANNEL;   cnt++)
            { 
              EEPROM.write(cnt+md_offset,md_data.md_channel_instrument[cnt]);
            }
            for (int cnt = 0; cnt<MD_MAX_CHANNEL; cnt++) 
            {
              EEPROM.write(MD_MAX_CHANNEL+cnt+md_offset,md_data.md_channel_volume[cnt]);
            }
            for (int cnt = 0; cnt<MD_MAX_CHANNEL; cnt++)
            {
              EEPROM.write((MD_MAX_CHANNEL*2)+cnt+md_offset,md_data.md_channel_volume[cnt]);
            }
            EEPROM.write((MD_MAX_CHANNEL*3)+md_offset,*((char*) &md_data.option));

            // Для всех каналов задать инструмент и громкость
            for (int cnt =0; cnt<MD_MAX_CHANNEL; cnt++)
            {
              // Если после чтения значения вышли за пределы - исправить
              if (md_data.md_channel_bank[cnt] >= pdev->banks_count) md_data.md_channel_bank[cnt] = 0;
              if (md_data.md_channel_instrument[cnt] > MD_MAX_INSTRUMENT) md_data.md_channel_instrument[cnt] = 0;
              if (md_data.md_channel_volume[cnt] > MD_MAX_VOLUME) md_data.md_channel_volume[cnt] = 64;
              // Задать параметры
              if (!(cnt == CHANNEL4 && pdev->has_drums_banks == 0)) {  // Можно сменить банк 
                Command3 (0xB0,cnt,0x00,pdev->banks[md_data.md_channel_bank[cnt]]);
                for (char a=0;a<100;a++);
              }
              Command2 (0xC0,cnt,md_data.md_channel_instrument[cnt]);
              for (char a=0;a<100;a++);
              Command3 (0xB0,cnt,0x07,md_data.md_channel_volume[cnt]);
              for (char a=0;a<100;a++);
            }
          }
        }

        play_key(i, j, 127);
      }
      // Если кнопка постоянно отжата, предотвращение залипания кнопки "Режим"
      if ((data[i] & (1<<j)))
      {
        if (func[i][j] == _MD) md_active = 0;
        if (func[i][j] == _C0) c0_active = 0;
        if (func[i][j] == _C1) c1_active = 0;
        if (func[i][j] == _C2) c2_active = 0;
        if (func[i][j] == _C3) c3_active = 0;
        if (func[i][j] == _C4) c4_active = 0;
        if (func[i][j] == _C5) c5_active = 0;
      }
    }  
  }  
  // Обносить предыдущее значение 
  datap[i] = data[i];
  // Отключить линию датчиков Холла
  // Если нужно добавить ещё линию, то добавьте ещё одну строку ниже:
  if (0 == i) PORTB &= ~(1<<L0_PB2); // Выключен L0
  if (1 == i) PORTB &= ~(1<<L1_PB4); // Выключен L1
  if (2 == i) PORTB &= ~(1<<L2_PB3); // Выключен L2
  if (3 == i) PORTB &= ~(1<<L3_PB0); // Выключен L3
  if (4 == i) PORTB &= ~(1<<L4_PB5); // Выключен L4
  if (5 == i) PORTD &= ~(1<<L5_PD4); // Выключен L5
  if (6 == i) PORTD &= ~(1<<L6_PD5); // Выключен L6
  if (7 == i) PORTD &= ~(1<<L7_PD2); // Выключен L7
  if (8 == i) PORTD &= ~(1<<L8_PD3); // Выключен L8
  if (9 == i) PORTB &= ~(1<<L9_PB1); // Выключен L9
  // Следующее значение линии L
  i++;
  // Если дошли до крайней линии, то начать с нулевой.
  // Если нужно сделать количество линий больше чем L9, то нужно увеличить это значение.
  if (i>9) i=0;
}

//  plays a MIDI note.  Doesn't check to see that
//  cmd is greater than 127, or that data values are  less than 127:
//  Функция проигрывающая ноту
void noteOn(int cmd, char channel, int pitch, int velocity) {
#ifndef DEBUG_PRESSURE
  Serial.write(cmd | _CH(channel));
  Serial.write(pitch);
  Serial.write(velocity);
#endif  
}

// Передать команду MIDI из двух байт.
void Command2 (char byte1, char channel, char byte2)
{
#ifndef DEBUG_PRESSURE
  Serial.write(byte1 | _CH(channel));
  Serial.write(byte2);
  for (volatile unsigned int a=0;a<50;a++);
#endif  
}

// Передать команду MIDI из трёх байт.
void Command3 (char byte1, char channel, char byte2, char byte3)
{
#ifndef DEBUG_PRESSURE
  Serial.write(byte1 | _CH(channel));
  Serial.write(byte2);
  Serial.write(byte3);
  for (volatile unsigned int a=0;a<50;a++);
#endif  
}

