// Пин, к которому подключен датчик (через резистор)
const int SENSOR_PIN = A0;

// Сопротивление вашего резистора в Омах.
const float RESISTOR_VALUE = 250.0;

// Максимальный диапазон измерения вашего датчика в метрах.
const float SENSOR_RANGE_METERS = 5.0;

// Опорное напряжение Arduino. Для Uno/Nano/Mega это обычно 5.0В.
const float V_REF = 5.0;

// Количество замеров для медианного фильтра.
const int NUM_SAMPLES = 15;

// --- КОНЕЦ НАСТРОЕК ---

// Массив для хранения замеров
int samples[NUM_SAMPLES];

void setup() {
  // Запускаем Serial порт для вывода данных на компьютер
  Serial.begin(9600);

  // Выводим приветственное сообщение и наши настройки
  Serial.println("--- Монитор датчика уровня воды (с медианным фильтром) ---");
  Serial.print("Количество замеров для фильтра: ");
  Serial.println(NUM_SAMPLES);
  Serial.println("---------------------------------------------------------");
  delay(1000);
}

void loop() {
  // --- Шаг 1: Собираем 12 быстрых замеров ---
  for (int i = 0; i < NUM_SAMPLES; i++) {
    samples[i] = analogRead(SENSOR_PIN);
    delay(20); // Небольшая пауза между замерами для стабилизации АЦП
  }

  // --- Шаг 2: Сортируем массив замеров по возрастанию ---
  // Мы используем простой алгоритм сортировки (пузырьковая сортировка).
  // Для 12 элементов он работает достаточно быстро.
  for (int i = 0; i < NUM_SAMPLES - 1; i++) {
    for (int j = 0; j < NUM_SAMPLES - i - 1; j++) {
      if (samples[j] > samples[j + 1]) {
        // меняем элементы местами
        int temp = samples[j];
        samples[j] = samples[j + 1];
        samples[j + 1] = temp;
      }
    }
  }

  // --- Шаг 3: Находим медианное значение ---
  // Медиана - это значение "посередине" отсортированного набора.
  float medianRawValue = samples[8];

  // --- Шаг 4: Преобразуем медианное значение в напряжение ---
  float voltage = (medianRawValue / 1023.0) * V_REF;

  // --- Шаг 5: Преобразуем напряжение в ток (в миллиамперах) ---
  float current_mA = (voltage / RESISTOR_VALUE) * 1000.0;

  // --- Шаг 6: Преобразуем ток в уровень воды (в метрах) ---
  float useful_current = current_mA - 4.0;
  const float current_range = 16.0;
  float waterLevel = (useful_current / current_range) * SENSOR_RANGE_METERS;

  if (waterLevel < 0) {
    waterLevel = 0.0;
  }

  // --- Шаг 7: Выводим все данные в Монитор порта ---
  Serial.print("Медианное значение ADC: ");
  Serial.print(medianRawValue, 1); // Выводим с 1 знаком после запятой

  Serial.print(" | Напряжение: ");
  Serial.print(voltage, 3);
  Serial.print(" В");

  Serial.print(" | Ток: ");
  Serial.print(current_mA, 2);
  Serial.print(" мА");

  Serial.print(" | Уровень воды: ");
  Serial.print(waterLevel, 2);
  Serial.println(" м");

  // Ждем две секунды перед следующим циклом замеров
  delay(2000);
}
