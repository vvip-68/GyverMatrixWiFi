void InitializeDfPlayer1() {
  mp3Serial.begin(9600);  
  isDfPlayerOk = dfPlayer.begin(mp3Serial, true, true);
  if (isDfPlayerOk) {    
    dfPlayer.setTimeOut(2000);
    dfPlayer.EQ(DFPLAYER_EQ_NORMAL);
    dfPlayer.volume(1);
  } 
}

void InitializeDfPlayer2() {    
  Serial.print(F("Инициализация MP3 плеера."));
  refreshDfPlayerFiles();    
  Serial.println(String(F("Звуков будильника найдено: ")) + String(alarmSoundsCount));
  Serial.println(String(F("Звуков рассвета найдено: ")) + String(dawnSoundsCount));
  isDfPlayerOk = alarmSoundsCount + dawnSoundsCount > 0;
}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number: "));
      Serial.print(value);
      Serial.println(F(". Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

bool isPlayerBusy() {
  return digitalRead(PIN_BUSY) == 0;
}

void refreshDfPlayerFiles() {
  // Чтение почему-то не всегда работает, иногда возвращает 0 или число от какого-то предыдущего запроса
  // Для того, чтобы наверняка считать значение - первое прочитанное игнорируем, потом читаем несколько раз до повторения.

  // Папка с файлами для будильника
  int cnt = 0, val = 0, new_val = 0; 
  do {
    val = dfPlayer.readFileCountsInFolder(1);     delay(10);
    new_val = dfPlayer.readFileCountsInFolder(1); delay(10);    
    if (val == new_val && val != 0) break;
    cnt++;
    delay(100);
    Serial.print(F("."));
  } while ((val = 0 || new_val == 0 || val != new_val) && cnt < 5);
  alarmSoundsCount = val;
  
  // Папка с файлами для рассвета
  cnt = 0, val = 0, new_val = 0; 
  do {
    val = dfPlayer.readFileCountsInFolder(2);     delay(10);
    new_val = dfPlayer.readFileCountsInFolder(2); delay(10);     
    if (val == new_val && val != 0) break;
    cnt++;
    delay(100);
    Serial.print(F("."));
  } while ((val = 0 || new_val == 0 || val != new_val) && cnt < 5);    
  dawnSoundsCount = val;
  Serial.println();  
}
