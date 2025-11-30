#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

// --- Настройки ---
const float RESISTOR_OHMS = 150.6; // Сопротивление нашего резистора в Омах

// Характеристики датчика 4-20 мА
const float CURRENT_MIN_MA = 3.95;//4.0;
const float CURRENT_MAX_MA = 20.0;

// Диапазон измерения твоего датчика (например, 0-5 метров)
const float SENSOR_LEVEL_MIN_METERS = 0.0;
const float SENSOR_LEVEL_MAX_METERS = 10.0; // Укажи максимальную глубину для твоей модели




#define MODEM_POWER_PIN     12
#define LED_PIN LED_BUILTIN


// Количество считываний для получения медианы
const int NUM_READINGS = 13;


/**
 * @brief Считывает показания с датчика, фильтрует их и возвращает уровень воды.
 *
 * Функция выполняет NUM_READINGS (13) измерений, сортирует их и возвращает
 * медианное (центральное) значение для устранения случайных выбросов.
 *
 * @return float - Уровень воды в метрах. Возвращает -1.0 в случае ошибки (обрыв цепи).
 */
float getActualWaterLevel() {
  float readings[NUM_READINGS]; // Массив для хранения 13 показаний

  // 1. Собираем 13 показаний
  for (int i = 0; i < NUM_READINGS; i++) {
    int16_t adc_raw = ads.readADC_SingleEnded(0);
    float volts = ads.computeVolts(adc_raw);
    float current_mA = (volts / RESISTOR_OHMS) * 1000.0;

    float levelMeters;
    if (current_mA < (CURRENT_MIN_MA - 0.5)) { // Проверка на обрыв цепи
      levelMeters = -1.0; // Ошибка или обрыв
    } else {
      // Используем функцию map для перевода диапазона тока в диапазон уровня
      long current_micro_amps = current_mA * 1000;
      long min_micro_amps = CURRENT_MIN_MA * 1000;
      long max_micro_amps = CURRENT_MAX_MA * 1000;

      long level_cm = map(current_micro_amps, min_micro_amps, max_micro_amps, SENSOR_LEVEL_MIN_METERS * 100, SENSOR_LEVEL_MAX_METERS * 100);
      levelMeters = level_cm / 100.0;
    }
    readings[i] = levelMeters;
    delay(20); // Небольшая задержка между считываниями для стабилизации АЦП
  }

  // 2. Сортируем массив показаний (простой "пузырьковый" метод)
  for (int i = 0; i < NUM_READINGS - 1; i++) {
    for (int j = 0; j < NUM_READINGS - i - 1; j++) {
      if (readings[j] > readings[j + 1]) {
        float temp = readings[j];
        readings[j] = readings[j + 1];
        readings[j + 1] = temp;
      }
    }
  }

  // 3. Возвращаем центральный элемент. Для 13 элементов это 7-й (индекс 6)
  return readings[NUM_READINGS / 2];
}




void setup(void) {
  Serial.begin(9600);
  Serial.println("Запуск системы измерения уровня (4-20 мА)...");

  ads.begin();
  // Усиление GAIN_TWO подходит для диапазона +/- 2.048V.
  // Наш сигнал 0.4-2.0V идеально в него вписывается.
  ads.setGain(GAIN_TWO);

  pinMode(MODEM_POWER_PIN, OUTPUT);
  digitalWrite(MODEM_POWER_PIN, HIGH);
  pinMode(LED_PIN, OUTPUT);
}

void loop(void) {

  // Просто вызываем нашу новую функцию, чтобы получить готовое значение
  float waterLevel = getActualWaterLevel();

  // Выводим результаты
  if (waterLevel < 0) {
    Serial.println("Уровень: ОШИБКА (проверьте подключение датчика)");
  } else {
    Serial.print("Уровень воды: ");
    Serial.print(waterLevel, 2); // Выводим с точностью до 2 знаков после запятой
    Serial.println(" м");
  }

  // Ждем 1 секунду перед следующим циклом измерений
  delay(1000);


  // // Читаем напряжение на резисторе
  // int16_t adc0 = ads.readADC_SingleEnded(0);
  // float volts = ads.computeVolts(adc0);

  // // Рассчитываем ток в миллиамперах (I = V / R)
  // float current_mA = (volts / RESISTOR_OHMS) * 1000.0;

  // // Рассчитываем уровень воды
  // float levelMeters;
  // if (current_mA < (CURRENT_MIN_MA - 0.5)) { // Проверка на обрыв цепи
  //   levelMeters = -1.0; // Ошибка или обрыв
  // } else {
  //   // Используем функцию map для перевода диапазона тока в диапазон уровня
  //   long current_micro_amps = current_mA * 1000;
  //   long min_micro_amps = CURRENT_MIN_MA * 1000;
  //   long max_micro_amps = CURRENT_MAX_MA * 1000;

  //   long level_cm = map(current_micro_amps, min_micro_amps, max_micro_amps, SENSOR_LEVEL_MIN_METERS * 100, SENSOR_LEVEL_MAX_METERS * 100);
  //   levelMeters = level_cm / 100.0;

  //   // Ограничиваем значения, чтобы не выходить за пределы
  //   if (levelMeters < SENSOR_LEVEL_MIN_METERS) {
  //     levelMeters = SENSOR_LEVEL_MIN_METERS;
  //   }
  //   if (levelMeters > SENSOR_LEVEL_MAX_METERS) {
  //     levelMeters = SENSOR_LEVEL_MAX_METERS;
  //   }
  // }

  // // Выводим результаты
  // Serial.print("Voltage: "); Serial.print(volts, 3); Serial.print("V");
  // Serial.print("\t Current: "); Serial.print(current_mA, 2); Serial.print("mA");

  // if (levelMeters < 0) {
  //   Serial.println("\t Level: ERROR (check connection)");
  // } else {
  //   Serial.print("\t Level: "); Serial.print(levelMeters, 2); Serial.println(" m");
  // }

  // delay(1000);
}






















