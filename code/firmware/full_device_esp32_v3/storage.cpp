#include "storage.h"
#include "globals.h"
#include <vector>
#include "water_lvl_settings.h"

PrevState getPrevState() {
  PrevState result;

  preferences.begin(FLASH_STORAGE_NAME, false);

  uint32_t bootCount = preferences.getUInt("boot_count", 0);
  bootCount++;
  preferences.putUInt("boot_count", bootCount);

  bool success = preferences.getUInt("success", false);

  preferences.end();

  result.bootCount = bootCount;
  result.success = success;

  return result;
}


void setSuccess(bool val) {
  preferences.begin(FLASH_STORAGE_NAME, false);
  preferences.putUInt("success", val);
  preferences.end();
}



// --- Функция №1: Добавить новую строку в лог-файл ---
// Принимает строку данных и дописывает ее в конец файла.
// Если файла нет, он будет создан автоматически.
void addCsvLine(const String& csvData) {
  File file = LittleFS.open(LOG_FILE_PATH, "a");

  if (!file) {
    Serial.println("Не удалось открыть файл для дозаписи");
    return;
  }

  if (file.println(csvData)) {
    Serial.print("Строка добавлена в лог: ");
    Serial.println(csvData);
  } else {
    Serial.println("Ошибка записи в файл.");
  }

  file.close();
}



// --- Подготовка пакета данных из файла ---
// Читает ВЕСЬ файл логов, берет до 5 ПОСЛЕДНИХ строк и объединяет их.
String prepareLogPayload() {
  if (!LittleFS.exists(LOG_FILE_PATH)) {
    return "";
  }

  File file = LittleFS.open(LOG_FILE_PATH, "r");
  if (!file) {
    Serial.println("Не удалось открыть лог-файл для чтения.");
    return "";
  }

  // Читаем все строки из файла в вектор. Для ESP32 это нормально по памяти.
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

  // Собираем итоговый пакет данных (payload)
  String payload = "";
  for (int i = startIndex; i < lines.size(); i++) {
    payload += lines[i];
    if (i < lines.size() - 1) {
      payload += "\n";
    }
  }

  Serial.print("Подготовлен пакет из ");
  Serial.print(numLinesToSend);
  Serial.println(" последних записей.");
  return payload;
}