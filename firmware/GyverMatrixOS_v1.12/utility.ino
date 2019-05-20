// служебные функции

// отображаем счёт для игр
void displayScore(byte score) {
  if (SCORE_SIZE == 0) {
    if (score > 9) drawDigit3x5(score / 10, WIDTH / 2 - 4, HEIGHT / 2 - 3, GLOBAL_COLOR_1);
    drawDigit3x5(score % 10, WIDTH / 2, HEIGHT / 2 - 3, GLOBAL_COLOR_2);
  } else {
    if (score > 9) drawDigit5x7(score / 10, WIDTH / 2 - 6, HEIGHT / 2 - 4, GLOBAL_COLOR_1);
    drawDigit5x7(score % 10, WIDTH / 2, HEIGHT / 2 - 4, GLOBAL_COLOR_2);
  }
  FastLED.show();
}

// шрифт 5х7
const uint8_t font5x7[][5] PROGMEM = {
  {0x3e, 0x51, 0x49, 0x45, 0x3e}, // 0 0x30 48
  {0x00, 0x42, 0x7f, 0x40, 0x00}, // 1 0x31 49
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 2 0x32 50
  {0x21, 0x41, 0x45, 0x4b, 0x31}, // 3 0x33 51
  {0x18, 0x14, 0x12, 0x7f, 0x10}, // 4 0x34 52
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 5 0x35 53
  {0x3c, 0x4a, 0x49, 0x49, 0x30}, // 6 0x36 54
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 7 0x37 55
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 8 0x38 56
  {0x06, 0x49, 0x49, 0x29, 0x1e}, // 9 0x39 57
};

// шрифт 3х5 квадратный
const uint8_t font3x5[][3] PROGMEM = {
  {0b11111, 0b10001, 0b11111},    // 0
  {0b00000, 0b11111, 0b00000},    // 1
  {0b10111, 0b10101, 0b11101},    // 2
  {0b10101, 0b10101, 0b11111},    // 3
  {0b11100, 0b00100, 0b11111},    // 4
  {0b11101, 0b10101, 0b10111},    // 5
  {0b11111, 0b10101, 0b10111},    // 6
  {0b10000, 0b10000, 0b11111},    // 7
  {0b11111, 0b10101, 0b11111},    // 8
  {0b11101, 0b10101, 0b11111},    // 9
};

// шрифт 3х5 скруглённый
const uint8_t font3x5_s[][3] PROGMEM = {
  {0b01111, 0b10001, 0b11110},    // 0
  {0b01000, 0b11111, 0b00000},    // 1
  {0b10011, 0b10101, 0b01001},    // 2
  {0b10001, 0b10101, 0b11111},    // 3
  {0b11100, 0b00100, 0b11111},    // 4
  {0b11101, 0b10101, 0b10111},    // 5
  {0b01111, 0b10101, 0b10111},    // 6
  {0b10000, 0b10011, 0b11100},    // 7
  {0b11111, 0b10101, 0b11111},    // 8
  {0b11101, 0b10101, 0b11110},    // 9
};

// нарисовать цифру шрифт 3х5 квадратный
void drawDigit3x5(byte digit, byte X, byte Y, CRGB color) {
  if (digit > 9) return;
  for (byte x = 0; x < 3; x++) {
    byte thisByte = pgm_read_byte(&(font3x5[digit][x]));
    for (byte y = 0; y < 5; y++) {
      if (x + X > WIDTH || y + Y > HEIGHT) continue;
      if (thisByte & (1 << y)) drawPixelXY(x + X, y + Y, color);
    }
  }
}

// нарисовать цифру шрифт 3х5 скруглённый
void drawDigit3x5_s(byte digit, byte X, byte Y, CRGB color) {
  if (digit > 9) return;
  for (byte x = 0; x < 3; x++) {
    byte thisByte = pgm_read_byte(&(font3x5_s[digit][x]));
    for (byte y = 0; y < 5; y++) {
      if (x + X > WIDTH || y + Y > HEIGHT) continue;
      if (thisByte & (1 << y)) drawPixelXY(x + X, y + Y, color);
    }
  }
}

// нарисовать цифру шрифт 5х7
void drawDigit5x7(byte digit, byte X, byte Y, CRGB color) {
  if (digit > 9) return;
  for (byte x = 0; x < 5; x++) {
    byte thisByte = pgm_read_byte(&(font5x7[digit][x]));
    for (byte y = 0; y < 7; y++) {
      if (x + X > WIDTH || y + Y > HEIGHT) continue;
      if (thisByte & (1 << 6 - y)) drawPixelXY(x + X, y + Y, color);
    }
  }
}

// Gamma коррекция (Defalt Gamma = 2.8)
const uint8_t PROGMEM gammaR[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,
  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,
  5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,
  9,  9, 10, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 14, 14, 14,
  15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
  23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29, 30, 31, 31, 32, 33,
  33, 34, 35, 36, 36, 37, 38, 39, 40, 40, 41, 42, 43, 44, 45, 46,
  46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
  62, 63, 65, 66, 67, 68, 69, 70, 71, 73, 74, 75, 76, 78, 79, 80,
  81, 83, 84, 85, 87, 88, 89, 91, 92, 94, 95, 97, 98, 99, 101, 102,
  104, 105, 107, 109, 110, 112, 113, 115, 116, 118, 120, 121, 123, 125, 127, 128,
  130, 132, 134, 135, 137, 139, 141, 143, 145, 146, 148, 150, 152, 154, 156, 158,
  160, 162, 164, 166, 168, 170, 172, 174, 177, 179, 181, 183, 185, 187, 190, 192,
  194, 196, 199, 201, 203, 206, 208, 210, 213, 215, 218, 220, 223, 225, 227, 230
};

const uint8_t PROGMEM gammaG[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

const uint8_t PROGMEM gammaB[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,
  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,
  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  7,  7,  7,  8,
  8,  8,  8,  9,  9,  9, 10, 10, 10, 10, 11, 11, 12, 12, 12, 13,
  13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 19,
  20, 20, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 27, 27, 28, 28,
  29, 30, 30, 31, 32, 32, 33, 34, 34, 35, 36, 37, 37, 38, 39, 40,
  40, 41, 42, 43, 44, 44, 45, 46, 47, 48, 49, 50, 51, 51, 52, 53,
  54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 69, 70,
  71, 72, 73, 74, 75, 77, 78, 79, 80, 81, 83, 84, 85, 86, 88, 89,
  90, 92, 93, 94, 96, 97, 98, 100, 101, 103, 104, 106, 107, 109, 110, 112,
  113, 115, 116, 118, 119, 121, 122, 124, 126, 127, 129, 131, 132, 134, 136, 137,
  139, 141, 143, 144, 146, 148, 150, 152, 153, 155, 157, 159, 161, 163, 165, 167,
  169, 171, 173, 175, 177, 179, 181, 183, 185, 187, 189, 191, 193, 196, 198, 200
};

// гамма-коррекция (более натуральные цвета)
uint32_t gammaCorrection(uint32_t color) {
  byte r = (color >> 16) & 0xFF;  // Extract the RR byte
  byte g = (color >> 8) & 0xFF;   // Extract the GG byte
  byte b = color & 0xFF;        // Extract the BB byte

  r = pgm_read_byte(&gammaR[r]);
  g = pgm_read_byte(&gammaG[g]);
  b = pgm_read_byte(&gammaB[b]);

  uint32_t newColor = ((long)(r & 0xff) << 16) + ((long)(g & 0xff) << 8) + ((long)b & 0xff);
  return newColor;
}

// gamma correction для expandColor
static const uint8_t PROGMEM
gamma5[] = {
  0x00, 0x01, 0x02, 0x03, 0x05, 0x07, 0x09, 0x0b,
  0x0e, 0x11, 0x14, 0x18, 0x1d, 0x22, 0x28, 0x2e,
  0x36, 0x3d, 0x46, 0x4f, 0x59, 0x64, 0x6f, 0x7c,
  0x89, 0x97, 0xa6, 0xb6, 0xc7, 0xd9, 0xeb, 0xff
},
gamma6[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08,
  0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x10, 0x12, 0x13,
  0x15, 0x17, 0x19, 0x1b, 0x1d, 0x20, 0x22, 0x25,
  0x27, 0x2a, 0x2d, 0x30, 0x33, 0x37, 0x3a, 0x3e,
  0x41, 0x45, 0x49, 0x4d, 0x52, 0x56, 0x5b, 0x5f,
  0x64, 0x69, 0x6e, 0x74, 0x79, 0x7f, 0x85, 0x8b,
  0x91, 0x97, 0x9d, 0xa4, 0xab, 0xb2, 0xb9, 0xc0,
  0xc7, 0xcf, 0xd6, 0xde, 0xe6, 0xee, 0xf7, 0xff
};

// преобразовать цвет из 16 битного в 24 битный
static uint32_t expandColor(uint16_t color) {
  return ((uint32_t)pgm_read_byte(&gamma5[ color >> 11       ]) << 16) |
         ((uint32_t)pgm_read_byte(&gamma6[(color >> 5) & 0x3F]) <<  8) |
         pgm_read_byte(&gamma5[ color       & 0x1F]);
}

// залить все
void fillAll(CRGB color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
}

// функция отрисовки точки по координатам X Y
void drawPixelXY(int8_t x, int8_t y, CRGB color) {
  if (x < 0 || x > WIDTH - 1 || y < 0 || y > HEIGHT - 1) return;
  int thisPixel = getPixelNumber(x, y) * SEGMENTS;
  for (byte i = 0; i < SEGMENTS; i++) {
    leds[thisPixel + i] = color;
  }
}

// функция получения цвета пикселя по его номеру
uint32_t getPixColor(int thisSegm) {
  int thisPixel = thisSegm * SEGMENTS;
  if (thisPixel < 0 || thisPixel > NUM_LEDS - 1) return 0;
  return (((uint32_t)leds[thisPixel].r << 16) | ((long)leds[thisPixel].g << 8 ) | (long)leds[thisPixel].b);
}

// функция получения цвета пикселя в матрице по его координатам
uint32_t getPixColorXY(int8_t x, int8_t y) {
  return getPixColor(getPixelNumber(x, y));
}

// **************** НАСТРОЙКА МАТРИЦЫ ****************
#if (CONNECTION_ANGLE == 0 && STRIP_DIRECTION == 0)
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y y

#elif (CONNECTION_ANGLE == 0 && STRIP_DIRECTION == 1)
#define _WIDTH HEIGHT
#define THIS_X y
#define THIS_Y x

#elif (CONNECTION_ANGLE == 1 && STRIP_DIRECTION == 0)
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y (HEIGHT - y - 1)

#elif (CONNECTION_ANGLE == 1 && STRIP_DIRECTION == 3)
#define _WIDTH HEIGHT
#define THIS_X (HEIGHT - y - 1)
#define THIS_Y x

#elif (CONNECTION_ANGLE == 2 && STRIP_DIRECTION == 2)
#define _WIDTH WIDTH
#define THIS_X (WIDTH - x - 1)
#define THIS_Y (HEIGHT - y - 1)

#elif (CONNECTION_ANGLE == 2 && STRIP_DIRECTION == 3)
#define _WIDTH HEIGHT
#define THIS_X (HEIGHT - y - 1)
#define THIS_Y (WIDTH - x - 1)

#elif (CONNECTION_ANGLE == 3 && STRIP_DIRECTION == 2)
#define _WIDTH WIDTH
#define THIS_X (WIDTH - x - 1)
#define THIS_Y y

#elif (CONNECTION_ANGLE == 3 && STRIP_DIRECTION == 1)
#define _WIDTH HEIGHT
#define THIS_X y
#define THIS_Y (WIDTH - x - 1)

#else
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y y
#pragma message "Wrong matrix parameters! Set to default"

#endif

// получить номер пикселя в ленте по координатам
uint16_t getPixelNumber(int8_t x, int8_t y) {
  if ((THIS_Y % 2 == 0) || MATRIX_TYPE) {               // если чётная строка
    return (THIS_Y * _WIDTH + THIS_X);
  } else {                                              // если нечётная строка
    return (THIS_Y * _WIDTH + _WIDTH - THIS_X - 1);
  }
}

// hex string to uint32_t
uint32_t HEXtoInt(String hexValue) {
  byte tens, ones, number1, number2, number3;
  tens = (hexValue[0] < '9') ? hexValue[0] - '0' : hexValue[0] - '7';
  ones = (hexValue[1] < '9') ? hexValue[1] - '0' : hexValue[1] - '7';
  number1 = (16 * tens) + ones;

  tens = (hexValue[2] < '9') ? hexValue[2] - '0' : hexValue[2] - '7';
  ones = (hexValue[3] < '9') ? hexValue[3] - '0' : hexValue[3] - '7';
  number2 = (16 * tens) + ones;

  tens = (hexValue[4] < '9') ? hexValue[4] - '0' : hexValue[4] - '7';
  ones = (hexValue[5] < '9') ? hexValue[5] - '0' : hexValue[5] - '7';
  number3 = (16 * tens) + ones;

  return ((uint32_t)number1 << 16 | (uint32_t)number2 << 8 | number3 << 0);
}

uint32_t CountTokens(String str, char separator) {

  uint32_t count = 0;
  int pos = 0;
  String l_str = str;

  l_str.trim();
  if (l_str.length() <= 0) return 0;
  pos = l_str.indexOf(separator);
  while (pos >= 0) {
    count++;
    pos = l_str.indexOf(separator, pos + 1);
  }
  return ++count;
}

String GetToken(String str, uint32_t index, char separator) {

  uint32_t count = CountTokens(str, separator);

  if (count <= 1 || index < 1 || index > count) return str;

  uint32_t pos_start = 0;
  uint32_t pos_end = str.length();

  count = 0;
  for (uint32_t i = 0; i < pos_end; i++) {
    if (str.charAt(i) == separator) {
      count++;
      if (count == index) {
        pos_end = i;
        break;
      } else {
        pos_start = i + 1;
      }
    }
  }
  return str.substring(pos_start, pos_end);
}
