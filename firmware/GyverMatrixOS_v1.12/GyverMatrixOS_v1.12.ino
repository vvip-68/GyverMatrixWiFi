// Скетч к проекту "Адресная матрица"
// Гайд по постройке матрицы: https://alexgyver.ru/matrix_guide/
// Страница базового проекта (схемы, описания): https://alexgyver.ru/GyverMatrixBT/
// Страница проекта на GitHub: https://github.com/vvip-68/GyverMatrixWiFi
// Автор: AlexGyver Technologies, 2018
// Дальнейшее развитие: vvip, 2019
// https://AlexGyAver.ru/

// GyverMatrixOS
// Версия прошивки 1.12, совместима с приложением GyverMatrixWiFi версии 1.13 и выше
// поддерживает микроконтроллеры на базе ESP8266 (NodeMCU, Wemos D1)

// ************************ МАТРИЦА *************************

#include "FastLED.h"

#define BRIGHTNESS 32         // стандартная маскимальная яркость (0-255)
#define CURRENT_LIMIT 10000   // лимит по току в миллиамперах, автоматически управляет яркостью (пожалей свой блок питания!) 0 - выключить лимит

#define WIDTH 16              // ширина матрицы
#define HEIGHT 16             // высота матрицы
#define SEGMENTS 1            // диодов в одном "пикселе" (для создания матрицы из кусков ленты)
#define DEVICE_TYPE 1         // Использование матрицы: 0 - свернута в трубу для лампы; 1 - плоская матрица в рамке   

#define COLOR_ORDER GRB       // порядок цветов на ленте. Если цвет отображается некорректно - меняйте. Начать можно с RGB

#define MATRIX_TYPE 0         // тип матрицы: 0 - зигзаг, 1 - параллельная
#define CONNECTION_ANGLE 0    // угол подключения: 0 - левый нижний, 1 - левый верхний, 2 - правый верхний, 3 - правый нижний
#define STRIP_DIRECTION 0     // направление ленты из угла: 0 - вправо, 1 - вверх, 2 - влево, 3 - вниз
                              // при неправильной настрйоке матрицы вы получите предупреждение "Wrong matrix parameters! Set to default"
                              // шпаргалка по настройке матрицы здесь! https://alexgyver.ru/matrix_guide/

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

#define USE_NOISE_EFFECTS 1 // крутые полноэкранные эффекты (0 нет, 1 да) СИЛЬНО ЖРУТ ПАМЯТЬ!!!
#define USE_FONTS 1         // использовать буквы (бегущая строка) (0 нет, 1 да)
#define OVERLAY_CLOCK 1     // часы на фоне всех эффектов и игр. Жрёт SRAM память!

// игры
#define USE_SNAKE 1         // игра змейка (0 нет, 1 да)
#define USE_TETRIS 1        // игра тетрис (0 нет, 1 да)
#define USE_MAZE 1          // игра лабиринт (0 нет, 1 да)
#define USE_RUNNER 1        // игра бегалка-прыгалка (0 нет, 1 да)
#define USE_FLAPPY 1        // игра flappy bird
#define USE_ARKAN 1         // игра арканоид

#define SMOOTH_CHANGE 0     // плавная смена режимов через чёрный

// ****************** ПИНЫ ПОДКЛЮЧЕНИЯ *******************
// пины подписаны согласно pinout платы, а не надписям на пинах!
// esp8266 - плату выбирал "Node MCU v3 (SP-12E Module)"
#define LED_PIN 2           // пин ленты

// ******************************** ДЛЯ РАЗРАБОТЧИКОВ ********************************
#define DEBUG 0
#define NUM_LEDS WIDTH * HEIGHT * SEGMENTS

CRGB leds[NUM_LEDS];

byte CLOCK_ORIENT = 0;     // 0 горизонтальные, 1 вертикальные

// Макрос центрирования отображения часов на матрице
#define CLOCK_X_H (byte(float(WIDTH - (4*3 + 3*1)) / 2 + 0.51)) // 4 цифры * (шрифт 3 пикс шириной) 3 + пробела между цифрами), /2 - в центр 
#define CLOCK_Y_H (byte(float(HEIGHT - 1*5) / 2 + 0.51))        // Одна строка цифр 5 пикс высотой  / 2 - в центр
#define CLOCK_X_V (byte(float(WIDTH - (2*3 + 1)) / 2 + 0.51))   // 2 цифры * (шрифт 3 пикс шириной) 1 + пробел между цифрами) /2 - в центр
#define CLOCK_Y_V (byte(float(HEIGHT - (2*5 + 1)) / 2 + 0.51))  // Две строки цифр 5 пикс высотой + 1 пробел между строкми / 2 - в центр

#define CAL_X (byte(float(WIDTH - (4*3 + 1)) / 2 ))             // 4 цифры * (шрифт 3 пикс шириной) 1 + пробел между цифрами) /2 - в центр
#define CAL_Y (byte(float(HEIGHT - (2*5 + 1)) / 2))             // Две строки цифр 5 пикс высотой + 1 пробел между строкми / 2 - в центр

byte CALENDAR_X = CAL_X;
byte CALENDAR_Y = CAL_Y;

byte CLOCK_X = CLOCK_X_H;     // Для вертикальных часов CLOCK_X_V и CLOCK_Y_V
byte CLOCK_Y = CLOCK_Y_H;
byte COLOR_MODE = 0;          // Режим цвета часов
//                               0 - монохромные цвета, определенные для режимов NORMAL_CLOCK_COLOR, CONTRAST_COLOR_1, CONTRAST_COLOR_2, CONTRAST_COLOR_3 в clock.ino
//                               1 - радужная смена (каждая цифра)
//                               2 - радужная смена (часы, точки, минуты)
//                               3 - заданные цвета (часы, точки, минуты) - HOUR_COLOR, DOT_COLOR, MIN_COLOR в clock.ino

// ID типа эффектов (тип группы - текст, игры имеют один ID типа на все подтипы)
#define MC_TEXT                  0
#define MC_CLOCK                 1
#define MC_GAME                  2
#define MC_NOISE_MADNESS         3
#define MC_NOISE_CLOUD           4
#define MC_NOISE_LAVA            5
#define MC_NOISE_PLASMA          6
#define MC_NOISE_RAINBOW         7
#define MC_NOISE_RAINBOW_STRIP   8
#define MC_NOISE_ZEBRA           9
#define MC_NOISE_FOREST         10
#define MC_NOISE_OCEAN          11
#define MC_SNOW                 12
#define MC_SPARKLES             13
#define MC_MATRIX               14
#define MC_STARFALL             15
#define MC_BALL                 16
#define MC_BALLS                17
#define MC_RAINBOW              18
#define MC_RAINBOW_DIAG         19
#define MC_FIRE                 20
#define MC_IMAGE                21
#define MC_DAWN_ALARM           22
#define MC_FILL_COLOR           23

#define MAX_EFFECT              24         // количество эффектов, определенных в прошивке
#define MAX_GAME                 6         // количество игр, определенных в прошивке

// не забудьте указать количество режимов для корректного переключения с последнего на первый
// количество кастомных режимов (которые переключаются сами или кнопкой)
#define MODES_AMOUNT 31   

// Порядок следования эффектов и игр в демо-режиме (см. customModes() в custom.ino) ID с 0 до MODES_AMOUNT-1
// Если требуется изменить порядок следования эффектов в демо-режиме - (а также добавить/удалить режимы) - редактируйте
// последовательность тут. Наличие - тут и в customModes() в custom.ino
#define DEMO_TEXT_0              0
#define DEMO_TEXT_1              1
#define DEMO_TEXT_2              2
#define DEMO_NOISE_MADNESS       3
#define DEMO_NOISE_CLOUD         4
#define DEMO_NOISE_LAVA          5
#define DEMO_NOISE_PLASMA        6
#define DEMO_NOISE_RAINBOW       7
#define DEMO_NOISE_RAINBOW_STRIP 8
#define DEMO_NOISE_ZEBRA         9
#define DEMO_NOISE_FOREST       10
#define DEMO_NOISE_OCEAN        11
#define DEMO_SNOW               12
#define DEMO_SPARKLES           13
#define DEMO_MATRIX             14
#define DEMO_STARFALL           15
#define DEMO_BALL               16
#define DEMO_BALLS              17
#define DEMO_RAINBOW            18
#define DEMO_RAINBOW_DIAG       19
#define DEMO_FIRE               20
#define DEMO_SNAKE              21
#define DEMO_TETRIS             22
#define DEMO_MAZE               23
#define DEMO_RUNNER             24
#define DEMO_FLAPPY             25
#define DEMO_ARKANOID           26
#define DEMO_CLOCK              27
#define DEMO_ANIMATION          28
#define DEMO_DAWN_ALARM         29  // Режим эффекта будильника "Рассвет"
#define DEMO_FILL_COLOR         30  // Заливка матрицы одним цветом
// ---------------------------------
#define DEMO_DAWN_ALARM_SPIRAL 253  // Специальный режим, вызывается из DEMO_DAWN_ALARM для ламп на круговой матрице - огонек по спирали
#define DEMO_DAWN_ALARM_SQUARE 254  // Специальный режим, вызывается из DEMO_DAWN_ALARM для плоских матриц - огонек по спирали на плоскости матрицы
// ---------------------------------

// Типы эффектов (см. выше), в которых могут отображаться часы в наложении
#if (OVERLAY_CLOCK == 1)
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
  MC_FIRE,
  MC_DAWN_ALARM,
  MC_FILL_COLOR,
};
#endif

// Сквозная нумерация (ID) эффектов в группе эффектов
#define EFFECT_BREATH               0
#define EFFECT_COLOR                1
#define EFFECT_SNOW                 2
#define EFFECT_BALL                 3
#define EFFECT_RAINBOW              4
#define EFFECT_RAINBOW_PIX          5
#define EFFECT_FIRE                 6
#define EFFECT_MATRIX               7
#define EFFECT_BALLS                8
#define EFFECT_CLOCK                9
#define EFFECT_STARFALL            10
#define EFFECT_SPARKLES            11
#define EFFECT_RAINBOW_DIAG        12
#define EFFECT_NOISE_MADNESS       13
#define EFFECT_NOISE_CLOUD         14
#define EFFECT_NOISE_LAVA          15
#define EFFECT_NOISE_PLASMA        16
#define EFFECT_NOISE_RAINBOW       17
#define EFFECT_NOISE_RAINBOW_STRIP 18
#define EFFECT_NOISE_ZEBRA         19
#define EFFECT_NOISE_FOREST        20
#define EFFECT_NOISE_OCEAN         21
#define EFFECT_ANIMATION           22
#define EFFECT_DAWN_ALARM          23
#define EFFECT_FILL_COLOR          24

// Сквозная нумерация (ID) игр в группе игр
#define GAME_SNAKE                  0
#define GAME_TETRIS                 1
#define GAME_MAZE                   2
#define GAME_RUNNER                 3
#define GAME_FLAPPY                 4
#define GAME_ARKANOID               5

// Список и порядок эффектов и игр, передаваймый в приложение на смартфоне. Данные списки попадают в комбобокс выбора, 
// чей индекс передается из приложения в контроллер матрицы для выбора, поэтому порядок должен соответствовать 
// спискам эффектов и игр, определенному выше
#define EFFECT_LIST F("Дыхание,Цвета,Снегопад,Шарик,Радуга,Радуга пикс,Огонь,The Matrix,Шарики,Часы,Звездопад,Конфетти,Радуга диагональная,Цветной шум,Облака,Лава,Плазма,Радужные переливы,Полосатые переливы,Зебра,Шумящий лес,Морской прибой,Анимация,Рассвет,Лампа")
#define ALARM_LIST  F("Снегопад,Шарик,Радуга,Огонь,The Matrix,Шарики,Звездопад,Конфетти,Радуга диагональная,Цветной шум,Облака,Лава,Плазма,Радужные переливы,Полосатые переливы,Зебра,Шумящий лес,Морской прибой,Анимация,Рассвет")
#define GAME_LIST   F("Змейка,Тетрис,Лабиринт,Runner,Flappy Bird,Арканоид")

// Соответствие индексов списка эффектов будильника ALARM_LIST индексам списка существующих эффектов EFFECT_LIST
const byte ALARM_LIST_IDX[] PROGMEM = {EFFECT_SNOW, EFFECT_BALL, EFFECT_RAINBOW, EFFECT_FIRE, EFFECT_MATRIX, EFFECT_BALLS,
                                       EFFECT_STARFALL, EFFECT_SPARKLES, EFFECT_RAINBOW_DIAG, EFFECT_NOISE_MADNESS, EFFECT_NOISE_CLOUD,
                                       EFFECT_NOISE_LAVA, EFFECT_NOISE_PLASMA, EFFECT_NOISE_RAINBOW, EFFECT_NOISE_RAINBOW_STRIP, 
                                       EFFECT_NOISE_ZEBRA, EFFECT_NOISE_FOREST, EFFECT_NOISE_OCEAN, EFFECT_ANIMATION, EFFECT_DAWN_ALARM};

#if (SMOOTH_CHANGE == 1)
  byte fadeMode = 4;
  boolean modeDir;
#endif

#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ALLOW_INTERRUPTS 0

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
bool useSoftAP = false;            // использовать режим точки доступа
bool wifi_connected = false;       // true - подключение к wifi сети выполнена  
bool ap_connected = false;         // true - работаем в режиме точки доступа;
bool wifi_print_ip = false;        // В качестве бегущей строки отображается текущий IP WiFi соединения
String wifi_current_ip = "";

#include <TimeLib.h>
boolean showDateInClock = true;    // Показывать дату при отображении часов
byte showDateDuration = 5;         // на 5 секунд
byte showDateInterval = 20;        // через каждые 20 секунд
byte showDateState = false;        // false - отображаются часы; true - отображается дата
long showDateStateLastChange = 0;  // Время, когда отображение часов сменилось на отображение календаря и наоборот

String runningText = "";
byte buttons = 4;                  // 0 - верх, 1 - право, 2 - низ, 3 - лево, 4 - не нажата - управление играми

bool specialMode = false;          // Спец.режим, включенный вручную со смартфона или с кнопок быстрого включения режима
bool specialClock = false;         // Спец.режим использует overlay часов
byte specialBrightness = 1;        // Яркость в спец.режиме
bool isNightClock = false;         // Включен режим ночных часов с минимальной яркостью 
bool isTurnedOff = false;          // Включен черный экран (т.е всё выключено) 
int8_t specialModeId = -1;         // Номер текущего спецрежима
bool useAutoBrightness = false;    // Использовать автоматическую регулировку яркости
byte autoBrightnessMin = 1;        // Минимальный уровень яркости при автоматической регулировке

bool isAlarming = false;           // Сработал будильник "рассвет"
bool isPlayAlarmSound = false;     // Сработал настоящий будильник - играется звук будильника
bool isAlarmStopped = false;       // Сработавший будильник "рассвет" остановлен пользователем

byte alarmWeekDay = 0;             // Битовая маска дней недели будильника
byte alarmDuration = 1;            // Проигрывать звук будильнике N минут после срабатывания (по окончанию рассвета)

byte alarmHour[7]   = {0,0,0,0,0,0,0};  // Часы времени срабатывания будильника
byte alarmMinute[7] = {0,0,0,0,0,0,0};  // Минуты времени срабатывания будильника

byte dawnHour = 0;                 // Часы времени начала рассвета
byte dawnMinute = 0;               // Минуты времени начала рассвета
byte dawnWeekDay = 0;              // День недели времени начала рассвета (0 - выключено, 1..7 - пн..вс)
byte dawnDuration = 0;             // Продолжительность "рассвета" по настройкам
byte realDawnDuration = 0;         // Продолжительность "рассвета" по вычисленому времени срабатывания будильника
bool useAlarmSound = false;        // Использовать звуки в будильнике
int8_t alarmSound = 0;             // Звук будильника - номер файла в папке SD:/01 [-1 не использовать; 0 - случайный; 1..N] номер файла
int8_t dawnSound = 0;              // Звук рассвета   - номер файла в папке SD:/02 [-1 не использовать; 0 - случайный; 1..N] номер файла
byte maxAlarmVolume = 30;          // Максимальная громкость будильника (1..30)
byte alarmEffect = EFFECT_DAWN_ALARM; // Какой эффект используется для будильника "рассвет". Могут быть обычные эффекты - их яркость просто будет постепенно увеличиваться

// Сервер не может инициировать отправку сообщения клиенту - только в ответ на запрос клиента
// Следующие две переменные хранят сообщения, формируемые по инициативе сервира и отправляются в ответ на ближайший запрос от клиента,
// например в ответ на периодический ping - в команде sendAcknowledge();
String cmd95 = "";                 // Строка, формируемая sendPageParams(95) для отправки по инициативе сервера
String cmd96 = "";                 // Строка, формируемая sendPageParams(96) для отправки по инициативе сервера

static const byte maxDim = max(WIDTH, HEIGHT);
int globalBrightness = BRIGHTNESS;
byte breathBrightness;
uint32_t globalColor = 0xffffff;   // цвет рисования при запуске белый
byte frameNum;

int scrollSpeed = D_TEXT_SPEED;    // скорость прокрутки текста бегущей строки
int gameSpeed = DEMO_GAME_SPEED;   // скорость в играх 
int effectSpeed = D_EFFECT_SPEED;  // скрость изменения эффекта

boolean BTcontrol = false;         // флаг: true - ручное управление эффектами и играми; false - в режиме Autoplay
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
int8_t thisMode = 0;               // текущий режим
boolean mazeMode = false;      
int8_t hrs = 0, mins = 0, secs = 0, aday = 1, amnth = 1;
int16_t ayear = 1970;
boolean dotFlag;
byte modeCode;                     // 0 бегущая, 1 часы, 2 игры, 3 нойс маднесс и далее, 21 гифка или картинка,
boolean fullTextFlag = false;
bool eepromModified = false;
String text;

#include <EEPROM.h>

#if (USE_FONTS == 1)
#include "fonts.h"
#endif

uint32_t idleTime = ((long)IDLE_TIME * 60 * 1000);      // минуты -> миллисек
uint32_t autoplayTime = ((long)AUTOPLAY_PERIOD * 1000); // секунды -> миллисек
uint32_t autoplayTimer;

#include "timerMinim.h"
timerMinim effectTimer(D_EFFECT_SPEED);  // Таймер скорости эффекта (шага выполнения эффекта)
timerMinim gameTimer(DEMO_GAME_SPEED);   // Таймер скорости игры (шага выполнения игры)
timerMinim scrollTimer(D_TEXT_SPEED);    // Таймер прокрутки текста эффекта бегущей строки
timerMinim changeTimer(70);              // Таймер шага плавной смены режима - Fade
timerMinim halfsecTimer(500);            // Полусекундный таймер точек часов
timerMinim idleTimer(idleTime);          // Таймер бездействия ручного управлениядля автоперехода а демо-режим 
timerMinim dawnTimer(1000);              // Таймер шага рассвета для будильника "рассвет" 
timerMinim alarmSoundTimer(4294967295);  // Таймер выключения звука будильника
timerMinim fadeSoundTimer(4294967295);   // Таймер плавного включения / выключения звука
timerMinim autoBrightnessTimer(500);     // Таймер отслеживания показаний датчика света при включенной авторегулировки яркости матрицы
timerMinim saveSettingsTimer(30000);     // Таймер отложенного сохранения настроек

#define DEFAULT_NTP_SERVER "time.nist.gov" // NTP сервер по умолчанию

#define DEFAULT_AP_NAME "MatrixAP"       // Имя точки доступа по умолчанию 
#define DEFAULT_AP_PASS "12341234"       // Пароль точки доступа по умолчанию
#define NETWORK_SSID ""                  // Имя WiFi сети
#define NETWORK_PASS ""                  // Пароль для подключения к WiFi сети

                                         // к длине +1 байт на \0 - терминальный символ. Это буферы для загрузки имен/пароля из EEPROM. Значения задаются в defiine выше
char apName[17] = "";                    // Имя сети в режиме точки доступа
char apPass[9]  = "";                    // Пароль подключения к точке доступа
char ssid[25] = "";                      // SSID (имя) вашего роутера (конфигурируется подключением через точку доступа и сохранением в EEPROM)
char pass[17] = "";                      // пароль роутера

WiFiUDP udp;
unsigned int localPort = 2390;           // local port to listen for UDP packets
byte IP_STA[] = {192, 168, 0, 106};      // Статический адрес в локальной сети WiFi

IPAddress timeServerIP;
#define NTP_PACKET_SIZE 48               // NTP время в первых 48 байтах сообщения
uint16_t SYNC_TIME_PERIOD = 60;          // Период синхронизации в минутах
byte packetBuffer[NTP_PACKET_SIZE];      // буффер для хранения входящих и исходящих пакетов

int8_t timeZoneOffset = 7;               // смещение часового пояса от UTC
long ntp_t = 0;                          // Время, прошедшее с запроса данных с NTP-сервера (таймаут)
byte ntp_cnt = 0;                        // Счетчик попыток получить данные от сервера
bool init_time = false;                  // Флаг false - время не инициализировано; true - время инициализировано
bool refresh_time = true;                // Флаг true - пришло время выполнить синхронизацию часов с сервером NTP
bool useNtp = true;                      // Использовать синхронизацию времени с NTP-сервером
char ntpServerName[31] = "";             // Используемый сервер NTP

timerMinim ntpSyncTimer(1000 * 60 * SYNC_TIME_PERIOD);            // Сверяем время с NTP-сервером через SYNC_TIME_PERIOD минут

#include <SoftwareSerial.h>
// D3 is RX of ESP8266, connect to TX of DFPlayer
// D4 is TX of ESP8266, connect to RX of DFPlayer module
#define SRX D4
#define STX D3

SoftwareSerial mp3Serial(SRX, STX);

#include "DFRobotDFPlayerMini.h"     // Установите в менеджере библиотек стандартную библиотеку DFRobotDFPlayerMini ("DFPlayer - A Mini MP3 Player For Arduino" )
#define PIN_BUSY D5 

DFRobotDFPlayerMini dfPlayer; 
bool isDfPlayerOk = false;
int16_t alarmSoundsCount = 0;        // Кол-во файлов звуков в папке '01' на SD-карте
int16_t dawnSoundsCount = 0;         // Кол-во файлов звуков в папке '02' на SD-карте
byte soundFolder = 0;
byte soundFile = 0;
int8_t fadeSoundDirection = 1;       // направление изменения громкости звука: 1 - увеличение; -1 - уменьшение
byte fadeSoundStepCounter = 0;       // счетчик шагов изменения громкости, которое осталось сделать

#define PIN_BTN D6                   // кнопка подключена сюда (PIN --- КНОПКА --- GND)
#include "GyverButton.h"
GButton butt(PIN_BTN);

#define HOLD_TIMEOUT 2000            // Время удержания кнопки перед выполнением действия ( + debounce time) суммарно - около 3 сек
bool isButtonHold = false;           // Кнопка нажата и удерживается
long hold_start_time = 0;            // Время обнаружения состояния "Конпка нажата и удерживается"

String TEXT_1 = "";
String TEXT_2 = "";
String TEXT_3 = "";

#include "GyverFilters.h"
#define PHOTO_PIN 0                  // пин фоторезистора
GFilterRA brightness_filter;         // фильтр для плавного изменения яркости

bool AM1_running = false;            // Режим 1 по времени - работает
byte AM1_hour = 0;                   // Режим 1 по времени - часы
byte AM1_minute = 0;                 // Режим 1 по времени - минуты
int8_t AM1_effect_id = -5;           // Режим 1 по времени - ID эффекта или -5 - выключено; -4 - выключить матрицу (черный экран); -3 - ночные часы, -2 - камин с часами, -1 - бегущая строка, 0 - случайныйб 1 и далее - эффект ALARM_LIST
bool AM2_running = false;            // Режим 2 по времени - работает
byte AM2_hour = 0;                   // Режим 2 по времени - часы
byte AM2_minute = 0;                 // Режим 2 по времени - минуты
int8_t AM2_effect_id = -5;           // Режим 2 по времени - ID эффекта или -5 - выключено; -4 - выключить матрицу (черный экран); -3 - ночные часы, -2 - камин с часами, -1 - бегущая строка, 0 - случайныйб 1 и далее - эффект ALARM_LIST

void setup() {
  
  pinMode(PIN_BUSY, INPUT);
  
  Serial.begin(115200);
  delay(10);
  
  Serial.println();
  
  EEPROM.begin(256);
  loadSettings();

  randomSeed(analogRead(1));    // пинаем генератор случайных чисел

  // Первый этап инициализации плеера - подключение и основные настройки  
  InitializeDfPlayer1();
     
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  // Подключиться к WiFi сети
  startWiFi();

  // Если режим точки тоступане используется и к WiFi сети подключиться не удалось- создать точку доступа
  if (!wifi_connected){
    WiFi.mode(WIFI_AP);
    startSoftAP();
  }

  if (useSoftAP && !ap_connected) startSoftAP();    

  // Сообщить UDP порт, на который ожидаются подключения
  if (wifi_connected || ap_connected) {
    Serial.print(F("UDP-сервер на порту "));
    Serial.println(localPort);
  }

  // UDP-клиент на указанном порту
  udp.begin(localPort);

  // Таймер бездействия
  if (idleTime == 0) // Таймер Idle  отключен
    idleTimer.setInterval(4294967295);
  else  
    idleTimer.setInterval(idleTime);
    
  dawnTimer.setInterval(4294967295);
  
  // настройки ленты
  FastLED.addLeds<WS2812, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(globalBrightness);
  if (CURRENT_LIMIT > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);
  FastLED.clear();
  FastLED.show();

  // Второй этап инициализации плеера - проверка наличия файлов звуков на SD карте  
  if (isDfPlayerOk) InitializeDfPlayer2();
  if (!isDfPlayerOk) Serial.println(F("MP3 плеер недоступен."));

  if (CLOCK_X < 0) CLOCK_X = 0;
  if (CLOCK_Y < 0) CLOCK_Y = 0;  

  /*
  butt.setDebounce(50);        // настройка антидребезга (по умолчанию 80 мс)
  butt.setTimeout(300);        // настройка таймаута на удержание (по умолчанию 500 мс)
  butt.setIncrStep(2);         // настройка инкремента, может быть отрицательным (по умолчанию 1)
  butt.setIncrTimeout(500);    // настройка интервала инкремента (по умолчанию 800 мс)
  */
  
  TEXT_1 = String(F("Матрица"));
  TEXT_2 = String(F("на адресных"));
  TEXT_3 = String(F("светодиодах"));

  brightness_filter.setCoef(0.1); // установка коэффициента фильтрации (0.0... 1.0). Чем меньше, тем плавнее фильтр  

  // Рассчитать время начала рассвета
  calculateDawnTime();
}

void loop() {
  
  bluetoothRoutine();
}

// -----------------------------------------

void startWiFi() { 
  
  WiFi.disconnect(true);
  wifi_connected = false;
  WiFi.mode(WIFI_AP_STA);
 
  // Пытаемся соединиться с роутером в сети
  if (strlen(ssid) > 0) {
    Serial.print(F("Подключение к "));
    Serial.print(ssid);

    if (IP_STA[0] + IP_STA[1] + IP_STA[2] + IP_STA[3] > 0) {
      WiFi.config(IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], IP_STA[3]),  // 192.168.0.106
                  IPAddress(IP_STA[0], IP_STA[1], IP_STA[2], 1),          // 192.168.0.1
                  IPAddress(255, 255, 255, 0));
    }              
    WiFi.begin(ssid, pass);
  
    // Проверка соединения (таймаут 10 секунд)
    for (int j = 0; j < 10; j++ ) {
      wifi_connected = WiFi.status() == WL_CONNECTED; 
      if (wifi_connected) {
        // Подключение установлено
        Serial.println();
        Serial.print(F("WiFi подключен. IP адрес: "));
        Serial.println(WiFi.localIP());
        break;
      }
      delay(1000);
      Serial.print(".");
    }
    Serial.println();
    
    if (!wifi_connected)
      Serial.println(F("Не удалось подключиться к сети WiFi."));
  }  
}

void startSoftAP() {
  WiFi.softAPdisconnect(true);
  ap_connected = false;

  Serial.print(F("Создание точки доступа "));
  Serial.print(apName);
  
  ap_connected = WiFi.softAP(apName, apPass);

  for (int j = 0; j < 10; j++ ) {    
    if (ap_connected) {
      Serial.println();
      Serial.print(F("Точка доступа создана. Сеть: '"));
      Serial.print(apName);
      // Если пароль совпадает с паролем по умолчанию - печатать для информации,
      // если был изменен пользователем - не печатать
      if (strcmp(apPass, "12341234") == 0) {
        Serial.print(F("'. Пароль: '"));
        Serial.print(apPass);
      }
      Serial.println(F("'."));
      Serial.print(F("IP адрес: "));
      Serial.println(WiFi.softAPIP());
      break;
    }    
    
    WiFi.enableAP(false);
    WiFi.softAPdisconnect(true);
    delay(1000);
    
    Serial.print(".");
    ap_connected = WiFi.softAP(apName, apPass);
  }  
  Serial.println();  
  
  if (!ap_connected) 
    Serial.println(F("Не удалось создать WiFi точку доступа."));
}
