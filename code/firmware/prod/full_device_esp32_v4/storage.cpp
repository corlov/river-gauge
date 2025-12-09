#include "storage.h"
#include "globals.h"
#include <vector>
#include "water_lvl_settings.h"
#include "water_lvl_types.h"
#include "water_lvl_init.h"
#include "errors.h"


PrevState loadAndIncrementBootState() {
  PrevState result;
  result.bootCount = -1;
  result.success = false;
  result.failCounter = 0;

  if (preferences.begin(FLASH_STORAGE_NAME, false)) {
    uint32_t bootCount = preferences.getUInt("boot_count", 0);
    bootCount++;
    preferences.putUInt("boot_count", bootCount);

    bool success = preferences.getBool("success", false);

    uint32_t failCounter = preferences.getUInt("failCounter", 0);    

    preferences.end();

    result.bootCount = bootCount;
    result.success = success;
    result.failCounter = failCounter;

  }

  return result;
}



void setSuccess(bool val) {
  if (preferences.begin(FLASH_STORAGE_NAME, false)) {
    preferences.putBool("success", val);

    uint32_t failCounter = preferences.getUInt("failCounter", 0);
    if (val) {
      preferences.putUInt("failCounter", 0);
    } else {
      preferences.putUInt("failCounter", failCounter+1);
    }
    preferences.end();
  }
}




void addCsvLine(const String& csvData) {
  if (!LittleFS.begin()) {
    if (LittleFS.format()) {
      Serial.println("Файловая система успешно отформатирована! Пожалуйста, перезагрузите устройство (нажмите кнопку Reset).");
      blinkErrorCode(ERR_CODE_FS_CORRUPT);
    }
    return;
  }

  File file = LittleFS.open(LOG_FILE_PATH, "a");

  if (!file) {
    blinkErrorCode(ERR_OPEN_LOG_FILE_2);
    Serial.println("Не удалось открыть файл для дозаписи");
    return;
  }

  if (!file.println(csvData)) {
    blinkErrorCode(ERR_WRITE_LOG_FILE);
    Serial.println("Ошибка записи в файл.");
  }

  file.close();
}



// Читает ВЕСЬ файл логов, берет до {N} ПОСЛЕДНИХ строк и объединяет их.
String prepareLogPayload() {
  if (!LittleFS.exists(LOG_FILE_PATH)) {
    return "";
  }

  File file = LittleFS.open(LOG_FILE_PATH, "r");
  if (!file) {
    blinkErrorCode(ERR_OPEN_LOG);
    Serial.println("Не удалось открыть лог-файл для чтения.");
    return "";
  }

  // Читаем все строки из файла в вектор.
  std::vector<String> lines;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      lines.push_back(line);
    }
  }
  file.close();

  if (lines.empty()) {
    return "";
  }

  // Определяем, сколько строк брать (не больше MAX_LINES_TO_SEND)
  int numLinesToSend = lines.size();
  if (numLinesToSend > MAX_LINES_TO_SEND) {
    numLinesToSend = MAX_LINES_TO_SEND;
  }

  // Находим индекс первой строки, которую нужно отправить
  int startIndex = lines.size() - numLinesToSend;

  // Собираем итоговый пакет данных (payload), самую последнюю строку не берем - она есть и так в посылке
  String payload = "";
  for (int i = startIndex; i < lines.size() - 1; i++) {
    payload += lines[i];
    if (i < lines.size() - 1) {
      payload += "\n";
    }
  }

  return payload;
}



String readStringSetting(const char* key, const char* defaultValue) {
  preferences.begin(SETTINGS_NAMESPACE, true);
  String value = preferences.getString(key, defaultValue);
  preferences.end();
  return value;
}

void writeStringSetting(const char* key, const String& value) {
  preferences.begin(SETTINGS_NAMESPACE, false);
  preferences.putString(key, value);
  preferences.end();
}

int readIntSetting(const char* key, int defaultValue) {
  preferences.begin(SETTINGS_NAMESPACE, true);
  int value = preferences.getInt(key, defaultValue);
  preferences.end();
  return value;
}

void writeIntSetting(const char* key, int value) {
  preferences.begin(SETTINGS_NAMESPACE, false);
  preferences.putInt(key, value);
  preferences.end();
}

float readFloatSetting(const char* key, float defaultValue) {
  preferences.begin(SETTINGS_NAMESPACE, true);
  float value = preferences.getFloat(key, defaultValue);
  preferences.end();
  return value;
}

void writeFloatSetting(const char* key, float value) {
  preferences.begin(SETTINGS_NAMESPACE, false);
  preferences.putFloat(key, value);
  preferences.end();
}