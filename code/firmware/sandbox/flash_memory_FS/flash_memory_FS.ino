// --- Библиотеки для обоих методов ---
#include <Preferences.h>
#include "FS.h"
#include "LittleFS.h"

// --- Структура и константы для Preferences ---
struct PrevState {
  uint32_t bootCount;
  bool success;
};
Preferences preferences;
const char* FLASH_STORAGE_NAME = "my_app_state";

void setup() {
  Serial.begin(9600);
  delay(1000);

  // Инициализируем генератор случайных чисел
  randomSeed(analogRead(0));

  // ==================================================
  // ЧАСТЬ 1: РАБОТА С PREFERENCES (NVS)
  // ==================================================
  Serial.println("\n--- 1. Работа с Preferences (NVS) ---");
  PrevState currentState = loadState();
  Serial.printf("Это запуск номер: %u\n", currentState.bootCount);
  Serial.printf("Предыдущий запуск был успешным? %s\n", currentState.success ? "Да" : "Нет");

  currentState.bootCount++;
  currentState.success = true;
  saveState(currentState);
  Serial.println("Новое состояние (счетчик +1) сохранено во Flash.");

  // ==================================================
  // ЧАСТЬ 2: РАБОТА С LITTLEFS С АВТО-ФОРМАТИРОВАНИЕМ
  // ==================================================
  Serial.println("\n--- 2. Работа с файловой системой LittleFS ---");

  // Пытаемся смонтировать файловую систему
  if (!LittleFS.begin()) {
    Serial.println("Ошибка монтирования LittleFS! Файловая система повреждена.");
    Serial.println("Попытка форматирования...");

    // Если монтирование не удалось, форматируем
    if (LittleFS.format()) {
      Serial.println("Файловая система успешно отформатирована!");
      Serial.println("Пожалуйста, перезагрузите устройство (нажмите кнопку Reset).");
    } else {
      Serial.println("КРИТИЧЕСКАЯ ОШИБКА: Не удалось отформатировать файловую систему.");
    }
    // Останавливаем выполнение, чтобы пользователь мог перезагрузить устройство
    while (1);
  }
  Serial.println("Файловая система успешно смонтирована.");


  const char* logFilePath = "/ship_log.txt";

  // --- Генерируем случайные данные для новой записи ---
  long randomNumber = random(1000, 9999);
  String logEntry = "System Event ID: " + String(randomNumber);
  Serial.printf("Новая запись для журнала: '%s'\n", logEntry.c_str());

  // Открываем файл для ДОБАВЛЕНИЯ записи ("a" - append)
  File logFile = LittleFS.open(logFilePath, "a");
  if (logFile) {
    logFile.printf("Запись #%u | %s\n", currentState.bootCount, logEntry.c_str());
    logFile.close();
    Serial.printf("Запись добавлена в файл %s\n", logFilePath);
  } else {
    Serial.println("Не удалось открыть лог-файл для дозаписи!");
  }

  // --- Читаем ВЕСЬ лог-файл, чтобы увидеть, как он растет ---
  logFile = LittleFS.open(logFilePath, "r");
  if (logFile) {
    Serial.println("\n--- Содержимое бортового журнала ---");
    while (logFile.available()) {
      Serial.write(logFile.read());
    }
    logFile.close();
    Serial.println("------------------------------------");
  }
}

void loop() {
  // Пусто
}

// --- Функции для работы с Preferences (без изменений) ---
PrevState loadState() {
  PrevState result;
  preferences.begin(FLASH_STORAGE_NAME, true);
  result.bootCount = preferences.getUInt("boot_count", 0);
  result.success = preferences.getBool("success", false);
  preferences.end();
  return result;
}

void saveState(PrevState stateToSave) {
  preferences.begin(FLASH_STORAGE_NAME, false);
  preferences.putUInt("boot_count", stateToSave.bootCount);
  preferences.putBool("success", stateToSave.success);
  preferences.end();
}
