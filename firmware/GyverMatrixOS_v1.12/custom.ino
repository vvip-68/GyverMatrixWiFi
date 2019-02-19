// свой список режимов

// ************************ НАСТРОЙКИ ************************
#define SHOW_FULL_TEXT 1    // не переключать режим, пока текст не покажется весь
#define SHOW_TEXT_ONCE 1    // показывать бегущий текст только 1 раз

/*
   Режимы:
    clockRoutine();       // часы на чёрном фоне

   Эффекты:
    sparklesRoutine();    // случайные цветные гаснущие вспышки
    snowRoutine();        // снег
    matrixRoutine();      // "матрица"
    starfallRoutine();    // звездопад (кометы)
    ballRoutine();        // квадратик
    ballsRoutine();       // шарики
    rainbowRoutine();     // радуга во всю матрицу горизонтальная
    rainbowDiagonalRoutine();   // радуга во всю матрицу диагональная
    fireRoutine();        // огонь

  Крутые эффекты "шума":
    madnessNoise();       // цветной шум во всю матрицу
    cloudNoise();         // облака
    lavaNoise();          // лава
    plasmaNoise();        // плазма
    rainbowNoise();       // радужные переливы
    rainbowStripeNoise(); // полосатые радужные переливы
    zebraNoise();         // зебра
    forestNoise();        // шумящий лес
    oceanNoise();         // морская вода

  Игры:
    snakeRoutine();     // змейка
    tetrisRoutine();    // тетрис
    mazeRoutine();      // лабиринт
    runnerRoutine();    // бегалка прыгалка
    flappyRoutine();    // flappy bird
    arkanoidRoutine();  // арканоид

  Бегущая строка:
    fillString("Ваш текст", цвет);    // цвет вида 0x00ff25 или CRGB::Red и проч. цвета
    fillString("Ваш текст", 1);       // радужный перелив текста
    fillString("Ваш текст", 2);       // каждая буква случайным цветом!

  Рисунки и анимации:
    loadImage(<название массива>);    // основная функция вывода картинки
    imageRoutine();                   // пример использования
    animation();                      // пример анимации

*/

// ************************* СВОЙ СПИСОК РЕЖИМОВ ************************
// список можно менять, соблюдая его структуру. Можно удалять и добавлять эффекты, ставить их в
// любой последовательности или вообще оставить ОДИН. Удалив остальные case и break. Cтруктура оч простая:
// case <номер>: <эффект>;
//  break;

void customModes() {
  switch (thisMode) {    
    case 0: 
      text = runningText == "" ? "Gyver Matrix" : runningText;
      fillString(text, CRGB::RoyalBlue);
      break;
    case 1: 
      text = runningText == "" ? "РАДУГА" : runningText;
      fillString(text, 1);
      break;
    case 2: 
      text = runningText == "" ? "RGB LED" : runningText;
      fillString(text, 2);
      break;
    case 3: madnessNoise();
      break;
    case 4: cloudNoise();
      break;
    case 5: lavaNoise();
      break;
    case 6: plasmaNoise();
      break;
    case 7: rainbowNoise();
      break;
    case 8: rainbowStripeNoise();
      break;
    case 9: zebraNoise();
      break;
    case 10: forestNoise();
      break;
    case 11: oceanNoise();
      break;
    case 12: snowRoutine();
      break;
    case 13: sparklesRoutine();
      break;
    case 14: matrixRoutine();
      break;
    case 15: starfallRoutine();
      break;
    case 16: ballRoutine();
      break;
    case 17: ballsRoutine();
      break;
    case 18: rainbowRoutine();
      break;
    case 19: rainbowDiagonalRoutine();
      break;
    case 20: fireRoutine();
      break;
    case 21: snakeRoutine();
      break;
    case 22: tetrisRoutine();
      break;
    case 23: mazeRoutine();
      break;
    case 24: runnerRoutine();
      break;
    case 25: flappyRoutine();
      break;
    case 26: arkanoidRoutine();
      break;
    case 27: clockRoutine();
      break;
    case 28: animation();
      break;
  }
}

// ********************* ОСНОВНОЙ ЦИКЛ РЕЖИМОВ *******************

static void nextMode() {
#if (SMOOTH_CHANGE == 1)
  fadeMode = 0;
  modeDir = true;
#else
  nextModeHandler();
#endif
}

static void prevMode() {
#if (SMOOTH_CHANGE == 1)
  fadeMode = 0;
  modeDir = false;
#else
  prevModeHandler();
#endif
}

void nextModeHandler() {
  thisMode++;
  if (thisMode >= MODES_AMOUNT) thisMode = 0;
  loadingFlag = true;
  gamemodeFlag = false;
  autoplayTimer = millis();
  setTimersForMode(thisMode);
  FastLED.clear();
  FastLED.show();
}

void prevModeHandler() {
  thisMode--;
  if (thisMode < 0) thisMode = MODES_AMOUNT - 1;
  loadingFlag = true;
  gamemodeFlag = false;
  autoplayTimer = millis();
  setTimersForMode(thisMode);
  FastLED.clear();
  FastLED.show();
}

void setTimersForMode(byte aMode) {
  if (aMode >= 0 && aMode < 3) {
    // Это бегущий текст  
    scrollSpeed = getScrollSpeed();
    scrollTimer.setInterval(scrollSpeed);
  } else {
    byte tmp_effect = mapModeToEffect(aMode);
    if (tmp_effect != 255) {
      effectSpeed = getEffectSpeed(tmp_effect);
      effectTimer.setInterval(effectSpeed);
    } else {
      byte tmp_game = mapModeToGame(aMode);
      if (tmp_effect != 255) {
        gameSpeed = DEMO_GAME_SPEED;
        gameTimer.setInterval(gameSpeed);
      }
    }
  }
}

int fadeBrightness;
int fadeStepCount = 10;     // За сколько шагов убирать/добавлять яркость при смене режимов
int fadeStepValue = 5;      // Шаг убавления яркости

#if (SMOOTH_CHANGE == 1)
void modeFader() {
  if (fadeMode == 0) {
    fadeMode = 1;
    fadeStepValue = fadeBrightness / fadeStepCount;
    if (fadeStepValue < 1) fadeStepValue = 1;
  } else if (fadeMode == 1) {
    if (changeTimer.isReady()) {
      fadeBrightness -= fadeStepValue;
      if (fadeBrightness < 0) {
        fadeBrightness = 0;
        fadeMode = 2;
      }
      FastLED.setBrightness(fadeBrightness);
    }
  } else if (fadeMode == 2) {
    fadeMode = 3;
    if (modeDir) nextModeHandler();
    else prevModeHandler();
  } else if (fadeMode == 3) {
    if (changeTimer.isReady()) {
      fadeBrightness += fadeStepValue;
      if (fadeBrightness > globalBrightness) {
        fadeBrightness = globalBrightness;
        fadeMode = 4;
      }
      FastLED.setBrightness(fadeBrightness);
    }
  }
}
#endif

void customRoutine() {
   
  if (!gamemodeFlag) {

    // Беугщая строка - таймер внутри fillString (runningText.ino)
    if (thisMode >=0 && thisMode < 3) { 
      customModes();
      FastLED.show();
    } 

    // Эффекты - возможно наложение часов
    else {
      
      if (effectTimer.isReady()) {
        
#if (OVERLAY_CLOCK == 1 && USE_CLOCK == 1)
        boolean loadFlag2;
        boolean needOverlay = modeCode != MC_TEXT && overlayAllowed();
        if (needOverlay) {
          if (!loadingFlag && needUnwrap()) {
            if (CLOCK_ORIENT == 0)
              clockOverlayUnwrapH(CLOCK_X, CLOCK_Y);
            else
              clockOverlayUnwrapV(CLOCK_X, CLOCK_Y);
          }
          if (loadingFlag) loadFlag2 = true;
        }
#endif

        customModes();                // режимы крутятся, пиксели мутятся

#if (OVERLAY_CLOCK == 1 && USE_CLOCK == 1)
        if (needOverlay) {
          if (CLOCK_ORIENT == 0)
            clockOverlayWrapH(CLOCK_X, CLOCK_Y);
          else  
            clockOverlayWrapV(CLOCK_X, CLOCK_Y);
          if (loadFlag2) {
            setOverlayColors();
            loadFlag2 = false;
          }
        }
        loadingFlag = false;
#endif
        FastLED.show();
      }
    }
  } 

  // Игры - таймер внутри игр
  else {
    customModes();
  }
  
  btnsModeChange();
}

void checkIdleState() {

#if (SMOOTH_CHANGE == 1)
  modeFader();
#endif

  
  if (idleState) {
    if (fullTextFlag && SHOW_TEXT_ONCE) {
      fullTextFlag = false;
      autoplayTimer = millis();
      if (AUTOPLAY) nextMode();
    }
    
    if (millis() - autoplayTimer > autoplayTime && AUTOPLAY) {    // таймер смены режима
      if (modeCode == MC_TEXT && SHOW_FULL_TEXT) {    // режим текста
        if (fullTextFlag) {
          fullTextFlag = false;
          autoplayTimer = millis();
          nextMode();
        }
      } else {
        autoplayTimer = millis();
        nextMode();
      }
    }
  } else {
    if (idleTimer.isReady() && modeCode != MC_CLOCK) {      // таймер холостого режима. 
      idleState = true;                                     // Если находимся в режиме часов - автоматически из Idle в демо-режим не переходить 
      autoplayTimer = millis();
      gameDemo = true;

      gameSpeed = DEMO_GAME_SPEED;
      gameTimer.setInterval(gameSpeed);

      BTcontrol = false;
      loadingFlag = true;
      runningFlag = false;
      controlFlag = false;                      // После начала игры пока не трогаем кнопки - игра автоматическая 
      drawingFlag = false;
      gamemodeFlag = false;
      gamePaused = false;

      AUTOPLAY = true;
      
      FastLED.clear();
      FastLED.show();
    }
  }  
}

#if (GET_WEATHER == 1)
void weatherRequest() {  
  #if (USE_CLOCK == 1)
    if (init_time == 0) return;
  #endif
  
  Serial.println("Weather request...");
  client.stop();
  
  if (client.connect(server, 80)) {
    Serial.println("Connected to port 80");
    //client.println("GET /data/2.5/weather?q=" + nameOfCity + "&APPID=" + apiKey + "&mode=json&units=metric&lang=ru HTTP/1.1");
    client.println("GET /data/2.5/forecast?q=" + nameOfCity + "&APPID=" + apiKey + "&mode=json&units=metric&cnt=2&lang=ru HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
    Serial.println("Weather request sent");
    weather_t = millis();
  }
  else {
    Serial.println("Weather server connection failed!");
  }
}

void parseWeather() {
  char c = 0;
  text = "";
  while (client.available()) {
    c = client.read();
    if (c == '{') {
      startJson = true;
      jsonend++;
    }
    if (c == '}') {
      jsonend--;
    }
    if (startJson == true) {
      text += c;
    }
    if (jsonend == 0 && startJson == true) {
      parseJson(text.c_str());
      init_weather = 1;
      weather_t = 0;
      startJson = false;
    }
  }
}

void parseJson(const char * jsonString) {
  const size_t bufferSize = 2*JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + 4*JSON_OBJECT_SIZE(1) + 3*JSON_OBJECT_SIZE(2) + 3*JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 2*JSON_OBJECT_SIZE(7) + 2*JSON_OBJECT_SIZE(8) + 720;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  
  JsonObject& root = jsonBuffer.parseObject(jsonString);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }
  JsonArray& list = root["list"];
  JsonObject& nowT = list[0];
  JsonObject& later = list[1];

  String city = root["city"]["name"];

  float tempNow = nowT["main"]["temp"];
  float humidityNow = nowT["main"]["humidity"];
  float windspeedNow = nowT["wind"]["speed"];
  float pressureNow = nowT["main"]["pressure"];
  pressureNow = pressureNow * HPaTomm;
  String weatherNow = nowT["weather"][0]["description"];

  float tempLater = later["main"]["temp"];
  float humidityLater = later["main"]["humidity"];
  float windspeedLater = later["wind"]["speed"];
  float pressureLater = later["main"]["pressure"];
  pressureLater = pressureLater * HPaTomm;
  String weatherLater = later["weather"][0]["description"];
  
  // Формируем строку с сообщением о погоде
  runningText = "Погода сейчас:"+weatherNow+" температура:"+ String(tempNow,1) +"C влажность:"+String(humidityNow,0)+"% ветер:"+String(windspeedNow,1)+"м/с давление:"+String(pressureNow,0)+"мм.рт.ст.";
  Serial.println(runningText);
}

#endif

#if (MCU_TYPE != 1)
void timeSet(boolean type, boolean dir) {    // type: 0-часы, 1-минуты, dir: 0-уменьшить, 1-увеличить
  if (type) {
    if (dir) hrs++;
    else hrs--;
  } else {
    if (dir) mins++;
    else mins--;
    if (mins > 59) {
      mins = 0;
      hrs++;
    }
    if (mins < 0) {
      mins = 59;
      hrs--;
    }
  }
  if (hrs > 23) hrs = 0;
  if (hrs < 0) hrs = 23;
}
#endif

void btnsModeChange() {
#if (USE_BUTTONS == 1)
  if (bt_set.clicked()) {
    if (gamemodeFlag) gameDemo = !gameDemo;
    if (gameDemo) {
      gameSpeed = DEMO_GAME_SPEED;
      gameTimer.setInterval(gameSpeed);
      AUTOPLAY = true;
    } else {
      gameSpeed = D_GAME_SPEED;
      gameTimer.setInterval(gameSpeed);
      AUTOPLAY = false;
    }
  }
  if (bt_set.holded()) {
    if (modeCode == MC_GAME)
      mazeMode = !mazeMode;
#if (USE_CLOCK == 1 && USE_RTC == 1)
    if (modeCode == MC_CLOCK) {    // вход в настройку часов
      clockSet = !clockSet;
      AUTOPLAY = false;
      secs = 0;
      if (!clockSet) rtc.adjust(DateTime(2014, 1, 21, hrs, mins, 0)); // установка нового времени в RTC
    }
#endif
  }

  // timeSet type: 0-часы, 1-минуты, dir: 0-уменьшить, 1-увеличить

  if (gameDemo) {
    if (bt_right.clicked()) {
      if (!clockSet) {
        autoplayTimer = millis();
        nextMode();
      } else {
#if (USE_CLOCK == 1 && USE_RTC == 1)        
  #if (USE_WIFI == 1)
        adjustTime(60);
  #else
        timeSet(1, 1);
  #endif
#endif
      }
    }

    if (bt_left.clicked()) {
      if (!clockSet) {
        autoplayTimer = millis();
        prevMode();
      } else {
#if (USE_CLOCK == 1 && USE_RTC == 1)        
  #if (USE_WIFI == 1)
        adjustTime(-60);
  #else
        timeSet(1, 0);
  #endif
#endif  
      }
    }

    if (bt_up.clicked()) {
      if (!clockSet) {
        AUTOPLAY = true;
        autoplayTimer = millis();
      } else {
#if (USE_CLOCK == 1 && USE_RTC == 1)        
  #if (USE_WIFI == 1)
        adjustTime(3600);
  #else
        timeSet(0, 1);
  #endif
#endif  
      }
    }
    if (bt_down.clicked()) {
      if (!clockSet) {
        AUTOPLAY = false;
      } else {
#if (USE_CLOCK == 1 && USE_RTC == 1)        
  #if (USE_WIFI == 1)
        adjustTime(-3600);
  #else
        timeSet(0, 0);
  #endif
#endif  
      }
    }

    if (bt_right.holding())
      if (changeTimer.isReady()) {
        if (!clockSet) {
          if (runningFlag) {
            scrollSpeed -= 2;
            if (scrollSpeed < D_TEXT_SPEED_MIN) scrollSpeed = D_TEXT_SPEED_MIN;
            saveScrollSpeed(scrollSpeed);
            scrollTimer.setInterval(scrollSpeed);
          } else if (effectsFlag) {
            effectSpeed -= 2;
            if (effectSpeed < D_EFFECT_SPEED_MIN) effectSpeed = D_EFFECT_SPEED_MIN;
            saveEffectSpeed(effect, effectSpeed);
            effectTimer.setInterval(effectSpeed);
          } else if (gamemodeFlag) {
            gameSpeed -= 2;
            if (gameSpeed < D_GAME_SPEED_MIN) gameSpeed = D_GAME_SPEED_MIN;
            saveGameSpeed(game, gameSpeed);
            gameTimer.setInterval(gameSpeed);
          }
        } else {
#if (USE_CLOCK == 1 && USE_RTC == 1)        
  #if (USE_WIFI == 1)
          adjustTime(60);
  #else
          timeSet(1, 1);
  #endif
#endif  
        }
      }
    if (bt_left.holding())
      if (changeTimer.isReady()) {
        if (!clockSet) {
          if (runningFlag) {
            scrollSpeed += 2;
            if (scrollSpeed > D_TEXT_SPEED_MAX) scrollSpeed = D_TEXT_SPEED_MAX;
            saveScrollSpeed(scrollSpeed);
            scrollTimer.setInterval(scrollSpeed);
          } else if (effectsFlag) {
            effectSpeed += 2;
            if (effectSpeed > D_EFFECT_SPEED_MAX) effectSpeed = D_EFFECT_SPEED_MAX;
            saveEffectSpeed(effect, effectSpeed);
            effectTimer.setInterval(effectSpeed);
          } else if (gamemodeFlag) {
            gameSpeed += 2;
            if (gameSpeed > D_GAME_SPEED_MAX) gameSpeed = D_GAME_SPEED_MAX;
            saveGameSpeed(game, gameSpeed);
            gameTimer.setInterval(gameSpeed);
          }
        } else {
#if (USE_CLOCK == 1 && USE_RTC == 1)        
  #if (USE_WIFI == 1)
          adjustTime(-60);
  #else
          timeSet(1, 0);
  #endif
#endif  
        }
      }
    if (bt_up.holding())
      if (changeTimer.isReady()) {
        if (!clockSet) {
          globalBrightness += 2;
          if (globalBrightness > 255) globalBrightness = 255;
          saveMaxBrightness(globalBrightness);
          FastLED.setBrightness(globalBrightness);
        } else {
#if (USE_CLOCK == 1 && USE_RTC == 1)        
  #if (USE_WIFI == 1)
          adjustTime(3600);
  #else
          timeSet(0, 1);
  #endif
#endif  
        }
      }
    if (bt_down.holding())
      if (changeTimer.isReady()) {
        if (!clockSet) {
          globalBrightness -= 2;
          if (globalBrightness < 0) globalBrightness = 0;
          saveMaxBrightness(globalBrightness);
          FastLED.setBrightness(globalBrightness);
        } else {
#if (USE_CLOCK == 1 && USE_RTC == 1)        
  #if (USE_WIFI == 1)
          adjustTime(-3600);
  #else
          timeSet(0, 0);
  #endif
#endif  
        }
      }
  }
#endif
}
