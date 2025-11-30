#include <Preferences.h>

// Наша структура данных
struct PrevState {
  uint32_t bootCount;
  bool success;
};

// Создаем объект для работы с NVS
Preferences preferences;

// Имя "пространства" в памяти, где будут храниться наши данные
const char* FLASH_STORAGE_NAME = "my_app_state";

void setup() {
  Serial.begin(9600);
  delay(5000);
  Serial.println("\n--- Пример работы с Preferences (NVS) ---");

  // --- Шаг 1: Загружаем предыдущее состояние из памяти ---
  PrevState currentState = loadState();

  Serial.printf("Это запуск номер: %u\n", currentState.bootCount);
  Serial.printf("Предыдущий запуск был успешным? %s\n", currentState.success ? "Да" : "Нет");

  // --- Шаг 2: Модифицируем данные ---
  // Увеличиваем счетчик запусков
  currentState.bootCount++;
  // Допустим, этот запуск мы хотим пометить как "успешный"
  if (currentState.bootCount % 2 == 0) {
    currentState.success = true;
  }
  else {
    currentState.success = false;
  }

  // --- Шаг 3: Сохраняем новое состояние в память ---
  saveState(currentState);
  Serial.println("Новое состояние сохранено во Flash.");
}



void loop() {
  // Пусто
}




// Функция для ЗАГРУЗКИ состояния из Flash
PrevState loadState() {
  PrevState result;

  // Открываем наше хранилище в режиме "только чтение" (true)
  preferences.begin(FLASH_STORAGE_NAME, true);

  // Читаем значения. Если ключа нет, возвращается значение по умолчанию (0 и false)
  result.bootCount = preferences.getUInt("boot_count", 0);
  result.success = preferences.getBool("success", false);

  // Закрываем хранилище
  preferences.end();

  return result;
}



// Функция для СОХРАНЕНИЯ состояния во Flash
void saveState(PrevState stateToSave) {
  // Открываем наше хранилище в режиме "чтение-запись" (false)
  preferences.begin(FLASH_STORAGE_NAME, false);

  // Записываем значения по ключам
  preferences.putUInt("boot_count", stateToSave.bootCount);
  preferences.putBool("success", stateToSave.success);

  // Закрываем хранилище
  preferences.end();
}
