// Скетч к проекту "Адресная матрица"
// Гайд по постройке матрицы: https://alexgyver.ru/matrix_guide/
// Страница проекта (схемы, описания): https://alexgyver.ru/GyverMatrixBT/
// Подробное описание прошивки: https://alexgyver.ru/gyvermatrixos-guide/
// Исходники на GitHub: https://github.com/AlexGyver/GyverMatrixBT/
// Нравится, как написан код? Поддержи автора! https://alexgyver.ru/support_alex/
// Автор: AlexGyver Technologies, 2018
// https://AlexGyAver.ru/

// GyverMatrixOS
// Версия прошивки 1.12, совместима с приложением GyverMatrixBT версии 1.13 и выше

// ************************ МАТРИЦА *************************
// если прошивка не лезет в Arduino NANO - отключай режимы! Строка 66 и ниже

#include "FastLED.h"

#define BRIGHTNESS 32         // стандартная маскимальная яркость (0-255)
#define CURRENT_LIMIT 4000    // лимит по току в миллиамперах, автоматически управляет яркостью (пожалей свой блок питания!) 0 - выключить лимит

#define WIDTH 16              // ширина матрицы
#define HEIGHT 16             // высота матрицы
#define SEGMENTS 1            // диодов в одном "пикселе" (для создания матрицы из кусков ленты)

#define COLOR_ORDER GRB       // порядок цветов на ленте. Если цвет отображается некорректно - меняйте. Начать можно с RGB

#define MATRIX_TYPE 0         // тип матрицы: 0 - зигзаг, 1 - параллельная
#define CONNECTION_ANGLE 0    // угол подключения: 0 - левый нижний, 1 - левый верхний, 2 - правый верхний, 3 - правый нижний
#define STRIP_DIRECTION 0     // направление ленты из угла: 0 - вправо, 1 - вверх, 2 - влево, 3 - вниз
// при неправильной настрйоке матрицы вы получите предупреждение "Wrong matrix parameters! Set to default"
// шпаргалка по настройке матрицы здесь! https://alexgyver.ru/matrix_guide/

#define MCU_TYPE 1            // микроконтроллер: 
//                            0 - AVR (Arduino NANO/MEGA/UNO)
//                            1 - ESP8266 (NodeMCU, Wemos D1)
//                            2 - STM32 (Blue Pill)

// ******************** ЭФФЕКТЫ И РЕЖИМЫ ********************
#define D_TEXT_SPEED 100      // скорость бегущего текста по умолчанию (мс)
#define D_TEXT_SPEED_MIN 10
#define D_TEXT_SPEED_MAX 255

#define D_EFFECT_SPEED 80     // скорость эффектов по умолчанию (мс)
#define D_EFFECT_SPEED_MIN 0
#define D_EFFECT_SPEED_MAX 255

#define D_GAME_SPEED 250      // скорость игр по умолчанию (мс)
#define D_GAME_SPEED_MIN 25
#define D_GAME_SPEED_MAX 375

#define D_GIF_SPEED 100       // скорость гифок (мс)
#define DEMO_GAME_SPEED 60    // скорость игр в демо режиме (мс)

boolean AUTOPLAY = 1;         // 0 выкл / 1 вкл автоматическую смену режимов (откл. можно со смартфона)
#define AUTOPLAY_PERIOD 30    // время между авто сменой режимов (секунды)
#define IDLE_TIME 10          // время бездействия кнопок или Bluetooth (в минутах) после которого запускается автосмена режимов и демо в играх

// о поддерживаемых цветах читай тут https://alexgyver.ru/gyvermatrixos-guide/
#define GLOBAL_COLOR_1 CRGB::Green    // основной цвет №1 для игр
#define GLOBAL_COLOR_2 CRGB::Orange   // основной цвет №2 для игр

#define SCORE_SIZE 0          // размер символов счёта в игре. 0 - маленький для 8х8 (шрифт 3х5), 1 - большой (шрифт 5х7)
#define FONT_TYPE 1           // (0 / 1) два вида маленького шрифта в выводе игрового счёта

// ************** ОТКЛЮЧЕНИЕ КОМПОНЕНТОВ СИСТЕМЫ (для экономии памяти) *************
// внимание! отключение модуля НЕ УБИРАЕТ его эффекты из списка воспроизведения!
// Это нужно сделать вручную во вкладке custom, удалив ненужные функции

#define USE_BUTTONS 1       // использовать физические кнопки управления играми (0 нет, 1 да)
#define BT_MODE 0           // использовать управление по блютус (0 нет, 1 да) - СИЛЬНО ЖРЁТ ПАМЯТЬ!!!
#define WIFI_MODE 1         // использовать управление по WiFi (0 нет, 1 да) - для MCU_TYPE == 1 
#define USE_NOISE_EFFECTS 1 // крутые полноэкранные эффекты (0 нет, 1 да) СИЛЬНО ЖРУТ ПАМЯТЬ!!!
#define USE_FONTS 1         // использовать буквы (бегущая строка) (0 нет, 1 да)
#define USE_CLOCK 1         // использовать часы (0 нет, 1 да)
#define OVERLAY_CLOCK 1     // часы на фоне всех эффектов и игр. Жрёт SRAM память!
#define USE_RTC 0           // использовать часы реального времени DS3231 (0 нет, 1 да)
#define USE_ANIMATION 1     // использовать эффект анимации (0 нет, 1 да)
#define USE_EEPROM 1        // Использовать сохранение настроек эффектов во флэш-памяти    
#define USE_WEATHER 0       // Использовать прогноз погоды с интернета

// игры
#define USE_SNAKE 1         // игра змейка (0 нет, 1 да)
#define USE_TETRIS 1        // игра тетрис (0 нет, 1 да)
#define USE_MAZE 1          // игра лабиринт (0 нет, 1 да)
#define USE_RUNNER 1        // игра бегалка-прыгалка (0 нет, 1 да)
#define USE_FLAPPY 1        // игра flappy bird
#define USE_ARKAN 1         // игра арканоид

#define SMOOTH_CHANGE 1     // плавная смена режимов через чёрный

// ****************** ПИНЫ ПОДКЛЮЧЕНИЯ *******************
// Arduino (Nano, Mega)
#if (MCU_TYPE == 0)
#define LED_PIN 6           // пин ленты
#define BUTT_UP 3           // кнопка вверх
#define BUTT_DOWN 5         // кнопка вниз
#define BUTT_LEFT 2         // кнопка влево
#define BUTT_RIGHT 4        // кнопка вправо
#define BUTT_SET 7          // кнопка выбор/игра

// пины подписаны согласно pinout платы, а не надписям на пинах!
// esp8266 - плату выбирал "Node MCU 1.0 (SP-12E Module)"
#elif (MCU_TYPE == 1)
#define LED_PIN 2           // пин ленты
#define BUTT_UP 14          // кнопка вверх
#define BUTT_DOWN 13        // кнопка вниз
#define BUTT_LEFT 0         // кнопка влево
#define BUTT_RIGHT 12       // кнопка вправо
#define BUTT_SET 15         // кнопка выбор/игра

// STM32 (BluePill) - плату выбирал STM32F103C
#elif (MCU_TYPE == 2)
#define LED_PIN PB12         // пин ленты
#define BUTT_UP PA1          // кнопка вверх
#define BUTT_DOWN PA3        // кнопка вниз
#define BUTT_LEFT PA0        // кнопка влево
#define BUTT_RIGHT PA2       // кнопка вправо
#define BUTT_SET PA4         // кнопка выбор/игра
#endif

// ******************************** ДЛЯ РАЗРАБОТЧИКОВ ********************************
#define DEBUG 0
#define NUM_LEDS WIDTH * HEIGHT * SEGMENTS

CRGB leds[NUM_LEDS];

#define USE_WIFI (WIFI_MODE == 1 && MCU_TYPE == 1)           // WiFI поддерживается только для NodeMCU, Wemos D1
#define GET_WEATHER (USE_WEATHER == 1 && USE_WIFI == 1)      // Прогноз погоды с интернета - только при наличии WiFi

#if (USE_CLOCK == 1)
byte CLOCK_ORIENT = 0;         // 0 горизонтальные, 1 вертикальные

#define CLOCK_X_H (byte(float(WIDTH - (4*3 + 3*1)) / 2 + 0.51)) // 4 цифры * (шрифт 3 пикс шириной) 3 + пробела между цифрами), /2 - в центр 
#define CLOCK_Y_H (byte(float(HEIGHT - 1*5) / 2 + 0.51))        // Одна строка цифр 5 пикс высотой  / 2 - в центр
#define CLOCK_X_V (byte(float(WIDTH - (2*3 + 1)) / 2 + 0.51))   // 2 цифры * (шрифт 3 пикс шириной) 1 + пробел между цифрами) /2 - в центр
#define CLOCK_Y_V (byte(float(HEIGHT - (2*5 + 1)) / 2 + 0.51))  // Две строки цифр 5 пикс высотой + 1 пробел между строкми / 2 - в центр

byte CLOCK_X = CLOCK_X_H;      // Для вертикальных часов CLOCK_X_V и CLOCK_Y_V
byte CLOCK_Y = CLOCK_Y_H;
byte COLOR_MODE = 0;           // Режим цвета часов
//                                0 - заданные ниже цвета
//                                1 - радужная смена (каждая цифра)
//                                2 - радужная смена (часы, точки, минуты)
#endif

#define MAX_EFFECT 22         // количество эффектов, определенных в прошивке
#define MAX_GAME 6            // количество игр, определенных в прошивке

#define MC_TEXT 0
#define MC_CLOCK 1
#define MC_GAME 2
#define MC_NOISE_MADNESS 3
#define MC_NOISE_CLOUD 4
#define MC_NOISE_LAVA 5
#define MC_NOISE_PLASMA 6
#define MC_NOISE_RAINBOW 7
#define MC_NOISE_RAINBOW_STRIP 8
#define MC_NOISE_ZEBRA 9
#define MC_NOISE_FOREST 10
#define MC_NOISE_OCEAN 11
#define MC_SNOW 12
#define MC_SPARKLES 13
#define MC_MATRIX 14
#define MC_STARFALL 15
#define MC_BALL 16
#define MC_BALLS 17
#define MC_RAINBOW 18
#define MC_RAINBOW_DIAG 19
#define MC_FIRE 20
#define MC_IMAGE 21

// эффекты, в которых отображаются часы в наложении
#if (USE_CLOCK == 1 && OVERLAY_CLOCK == 1)
byte overlayList[] = {
  MC_NOISE_MADNESS,
  MC_NOISE_CLOUD,
  MC_NOISE_LAVA,
  MC_NOISE_PLASMA,
  MC_NOISE_RAINBOW,
  MC_NOISE_RAINBOW_STRIP,
  MC_NOISE_ZEBRA,
  MC_NOISE_FOREST,
  MC_NOISE_OCEAN,
  MC_SNOW,
  MC_SPARKLES,
  MC_MATRIX,
  MC_STARFALL,
  MC_BALL,
  MC_BALLS,
  MC_RAINBOW,
  MC_RAINBOW_DIAG,
  MC_FIRE
};
#endif

#if (SMOOTH_CHANGE == 1)
byte fadeMode = 4;
boolean modeDir;
#endif

#if (MCU_TYPE == 1)
  #define FASTLED_INTERRUPT_RETRY_COUNT 0
  #define FASTLED_ALLOW_INTERRUPTS 0
  #include <ESP8266WiFi.h>
  #include <WiFiUdp.h>

  #if (USE_CLOCK == 1)
    #include <OldTime.h>
  #endif

  #if (GET_WEATHER == 1)
    #include <ArduinoJson.h> // Качаем библиотеку для парсинга данных о погоде версии 5.xx. Версия 6.xx - не совместима
    #include <WiFiClient.h>  // Для запроса о погоде
  #endif
#endif

String runningText = "";

static const byte maxDim = max(WIDTH, HEIGHT);
byte buttons = 4;   // 0 - верх, 1 - право, 2 - низ, 3 - лево, 4 - не нажата
int globalBrightness = BRIGHTNESS;
byte breathBrightness;
uint32_t globalColor = 0xffffff;   // цвет рисования при запуске белый
byte frameNum;

int scrollSpeed = D_TEXT_SPEED;    // скорость прокрутки текста бегущей строки
int gameSpeed = DEMO_GAME_SPEED;   // скорость в играх 
int effectSpeed = D_EFFECT_SPEED;  // скрость изменения эффекта

boolean loadingFlag = true;
boolean runningFlag;               // текущий режим ручном режиме - бегущая строка
boolean drawingFlag;               // текущий режим в ручном режиме - рисование или картинка на матрице
boolean effectsFlag;               // текущий режим в ручном режиме - эффекты
boolean gamemodeFlag = false;      // текущий режим - игра
boolean controlFlag = false;       // управление игрой перехвачено с кнопок или смартфона
boolean gamePaused;                // игра приостановлена

byte game;                         // индекс текущей игры в ручном режиме
byte effect;                       // индекс текущего эффекта в ручном режиме

boolean gameDemo = true;           // игра в демо-режиме
boolean idleState = true;          // флаг холостого режима работы
boolean BTcontrol = false;         // флаг контроля с блютус или WiFi. Если false - управление с кнопок
int8_t thisMode = 0;               // текущий режим
boolean mazeMode = false;      
int8_t hrs = 0, mins = 0, secs = 0, aday = 1, amnth = 1;
int16_t ayear = 1970;
boolean dotFlag;
byte modeCode;                     // 0 бегущая, 1 часы, 2 игры, 3 нойс маднесс и далее, 21 гифка или картинка,
boolean fullTextFlag = false;
boolean clockSet = false;

bool eepromModified = false;
#if (USE_EEPROM == 1)
#include <EEPROM.h>
#endif

#if (USE_FONTS == 1)
#include "fonts.h"
#endif

uint32_t idleTime = ((long)IDLE_TIME * 60 * 1000);      // минуты -> миллисек
uint32_t autoplayTime = ((long)AUTOPLAY_PERIOD * 1000); // секунды -> миллисек
uint32_t autoplayTimer;

#include "timerMinim.h"
timerMinim effectTimer(D_EFFECT_SPEED);
timerMinim gameTimer(DEMO_GAME_SPEED);
timerMinim scrollTimer(D_TEXT_SPEED);
timerMinim changeTimer(70);
timerMinim halfsecTimer(500);
timerMinim idleTimer(idleTime);

#include "bitmap2.h"          // файлы с картинками анимации

// не забудьте указать количество режимов для корректного переключения с последнего на первый
// количество кастомных режимов (которые переключаются сами или кнопкой)
#if (USE_ANIMATION == 1 && WIDTH == 16 && HEIGHT == 16)  // Анимация в bitmap.h - фреймы подготовлены только для матрицы 16x16
#define MODES_AMOUNT 29   
#else
#define MODES_AMOUNT 28
#endif

#if (USE_CLOCK == 1 && (MCU_TYPE == 0 || MCU_TYPE == 1))
  #include "RTClib.h"
  #if (USE_RTC == 1)
  #include <Wire.h>
  RTC_DS3231 rtc;
  // RTC_DS1307 rtc;
  #endif
#endif

  String text;

#if (MCU_TYPE == 1)

// Раскомментируйте следующую строку, если параметры подключения к WiFiсерверу задаются
// явным образом в блоке ниже. Если строка закомментирована - блок определения параметров подключения в
// точно таком же формате вынесен в отдельный файл 'WiFiNet.h' и переменные при сборке скетча будут браться из него.

// #define public
#if (USE_WIFI == 1)
  #ifndef public 
    #include "WiFiNet.h"          // приватные данные и пароли доступа к WiFi сети
  #else
    char ssid[] = "****";       // SSID (имя) вашего роутера
    char pass[] = "****";       // пароль роутера

    // Регистрационные данные к погодному серверу
    #if (GET_WEATHER == 1)
      const char server[] = "api.openweathermap.org"; // Сервер для получения погоды
      String nameOfCity = "london,uk";                // Задаем город и страну через зяпятую
      String apiKey = "*****";                        // Указываем свой ключ, полученный при регистрации на openweathermap.org
    #endif
  #endif
  WiFiUDP udp;
  unsigned int localPort = 2390;       // local port to listen for UDP packets
#endif

#if (USE_CLOCK == 1 || GET_WEATHER == 1)
  timerMinim WifiTimer(500);  
#endif 

#if (USE_CLOCK == 1)
  const char* ntpServerName = "time.nist.gov";
  IPAddress timeServerIP;
  #define NTP_PACKET_SIZE 48           // NTP время в первых 48 байтах сообщения
  uint16_t SYNC_TIME_PERIOD = 60;      // Период синхронизации в минутах
  byte packetBuffer[NTP_PACKET_SIZE];  // буффер для хранения входящих и исходящих пакетов

  int8_t timeZoneOffset = 7;           // set this to the offset in hours to your local time;
  long ntp_t = 0;
  byte init_time = 0;
  bool useNtp = true;
  
  timerMinim ntpTimer(1000 * 60 * SYNC_TIME_PERIOD);            // Сверяем время через SYNC_TIME_PERIOD минут
#endif

#if (GET_WEATHER == 1)
  WiFiClient client;

  #define SYNC_WEATHER_PERIOD 60
  long weather_t = 0;
  byte init_weather = 0;

  int jsonend = 0;
  boolean startJson = false;
  int status = WL_IDLE_STATUS;

  #define JSON_BUFF_DIMENSION 2500
  #define HPaTomm 0.7500637554192

  timerMinim WeatherCheck(1000 * 60 * SYNC_WEATHER_PERIOD);
#endif
  
#endif

void setup() {
#if (BT_MODE == 1)
  Serial.begin(9600);
  delay(10);
#endif

#if (USE_EEPROM == 1)
  #if (MCU_TYPE == 1)
  EEPROM.begin(256);
  #endif
  loadSettings();
#endif
  
#if (MCU_TYPE == 1 && USE_WIFI == 1)
// Если BT не используется - в Serial выводятся диагностические сообщения.
// Если BT используется - Serial исполльзуется для коммуникации через BT
#if (BT_MODE == 0)
  Serial.begin(115200);
  delay(10);
#endif
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid, pass);
  delay(100);
  udp.begin(localPort);
#endif

#if (USE_CLOCK == 1 && USE_RTC == 1 && (MCU_TYPE == 0 || MCU_TYPE == 1))
  rtc.begin();
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  DateTime now = rtc.now();
  secs = now.second();
  mins = now.minute();
  hrs = now.hour();
#endif

  // Таймер бездействия
  if (idleTime == 0) // Таймер Idle  отключен
    idleTimer.setInterval(4294967295);
  else  
    idleTimer.setInterval(idleTime);
  
  // настройки ленты
  FastLED.addLeds<WS2812, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(globalBrightness);
  if (CURRENT_LIMIT > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);
  FastLED.clear();
  FastLED.show();
  randomSeed(analogRead(0) + analogRead(1));    // пинаем генератор случайных чисел

#if (USE_CLOCK == 1)
  if (CLOCK_X < 0) CLOCK_X = 0;
  if (CLOCK_Y < 0) CLOCK_Y = 0;  
#endif  
}

void loop() {
  checkWiFiConnection(); 
  bluetoothRoutine();
}

// -----------------------------------------

#if (MCU_TYPE == 1 && USE_WIFI == 1)
bool wifi_connected = false;
bool printed_1 = false;
bool printed_2 = false;

void checkWiFiConnection() {
  
  // Проверяем подключение к WiFi, при необходимости (пере)подключаемся к сети
  wifi_connected = WiFi.status() == WL_CONNECTED; 
  if (!wifi_connected) {
    if (!printed_1)
    {      
#if (BT_MODE == 0)
      Serial.print("Connecting to ");
      Serial.print(ssid);
      Serial.println("...");
#endif  
      printed_1 = true;
      printed_2 = false;
    }
  }

  // Сразу после подключения - печатаем результат подключения
  if (wifi_connected && !printed_2) {
#if (BT_MODE == 0)
    Serial.print("WiFi подключен. IP адрес: ");
    Serial.println(WiFi.localIP());
    Serial.printf("UDP-сервер на порту %d\n", localPort);
#endif  
    printed_1 = false;
    printed_2 = true;
  }
}
#else
void checkWiFiConnection() { } 
#endif  
