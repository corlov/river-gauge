#include <OneWire.h>
#include <DallasTemperature.h>

#define WATER_LEVEL_PIN     14
#define SHUNT_RESISTOR      150.6
#define SENSOR_RANGE_METERS 10.0
#define V_REF               3.3
#define MODEM_POWER_PIN     12
#define SAMPLES_COUNT 50

#define WATER_LEVEL_CORRECTION_FACTOR 1.136
#define SENSOR_ZERO_CURRENT 3.71


#define WATER_TEMPERATURE_PIN 4 // <-- НОВЫЙ ПИН для датчика температуры


portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;



#define LED_PIN LED_BUILTIN

OneWire oneWire(WATER_TEMPERATURE_PIN);
DallasTemperature sensors(&oneWire);



void setup() {
  Serial.begin(9600);
  delay(1000);
  analogSetPinAttenuation(WATER_LEVEL_PIN, ADC_11db);
  pinMode(MODEM_POWER_PIN, OUTPUT);
  digitalWrite(MODEM_POWER_PIN, HIGH);

  // Инициализация датчика температуры
  sensors.begin();

  pinMode(LED_PIN, OUTPUT);
}




void loop() {
  float waterLevel = getWaterLevelSetup();
  float waterTemp = getWaterTemperature();

  // Выводим все в одной строке
  Serial.print("Уровень воды: ");
  Serial.print(waterLevel, 3);
  Serial.print(" метров | Температура: ");

  if (waterTemp != -999.0) { // Проверяем, что датчик не вернул ошибку
    Serial.print(waterTemp, 2);
    Serial.print(" °C");
  } else {
    Serial.print("Ошибка датчика");
  }

  Serial.print("\n");

  
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(1500);
}





float getWaterLevelSetup() {
  int samples[SAMPLES_COUNT];

  portENTER_CRITICAL(&mux);

  for (int i = 0; i < SAMPLES_COUNT; i++) {
    samples[i] = analogRead(WATER_LEVEL_PIN);
  }

  portEXIT_CRITICAL(&mux);
  
  for (int i = 0; i < SAMPLES_COUNT - 1; i++) {
    for (int j = 0; j < SAMPLES_COUNT - i - 1; j++) {
      if (samples[j] > samples[j + 1]) {
        int temp = samples[j];
        samples[j] = samples[j + 1];
        samples[j + 1] = temp;
      }
    }
  }

  int medianRawValue = samples[SAMPLES_COUNT / 2];

  Serial.print("  [Отладка: Медианное значение АЦП = ");
  Serial.print(medianRawValue);
  Serial.print("]  ");

  float voltage = (medianRawValue / 4095.0) * V_REF;
  float current_mA = (voltage / SHUNT_RESISTOR) * 1000.0;

  // --- НАША ДИАГНОСТИЧЕСКАЯ СТРОКА ---
  Serial.print("Ток: ");
  Serial.print(current_mA, 2); // Выводим ток с двумя знаками после запятой
  Serial.print(" мА  ");
  // ------------------------------------

  //float useful_current = current_mA - 4.0;
  float useful_current = current_mA - SENSOR_ZERO_CURRENT;
  const float current_range = 16.0;
  float waterLevel = (useful_current / current_range) * SENSOR_RANGE_METERS;

  if (waterLevel < 0) {
    waterLevel = 0.0;
  }

  return waterLevel * WATER_LEVEL_CORRECTION_FACTOR;
}


float getWaterTemperature() {
  sensors.requestTemperatures(); // Отправляем команду на измерение
  float tempC = sensors.getTempCByIndex(0); // Считываем температуру с первого датчика на шине

  // Проверка на ошибку чтения (-127 - это код ошибки датчика)
  if (tempC == DEVICE_DISCONNECTED_C) {
    return -999.0; // Возвращаем специальное значение, если датчик не найден
  }
  return tempC;
}


// Шаг 1: Калибровка "Нуля"
// Цель: Найти, какому току в миллиамперах соответствует нулевой уровень воды для твоего конкретного датчика. В теории это 4.0 мА, но на практике всегда есть погрешность.

// ДЕЙСТВИЕ: Вытащи датчик из воды и протри его. Он должен быть в воздухе. Это его состояние "0% уровня".

// НАБЛЮДЕНИЕ: Посмотри в Монитор порта. Ты увидишь строку, например:
// [Отладка: ... ] Ток: 3.98 мА Уровень воды: 0.000 метров

// ЧТО БРАТЬ: Нас интересует значение тока. В нашем примере это 3.98. Это и есть твой реальный "ноль".

// КУДА ВСТАВЛЯТЬ:

// В начало твоего скетча, где все #define, добавь новую строку:
// cpp
// #define SENSOR_ZERO_CURRENT 3.98 // Реальный ток при нулевом уровне
// Теперь в функции getWaterLevel найди строку:
// float useful_current = current_mA - 4.0;
// И замени "магическое число" 4.0 на твою новую константу:
// float useful_current = current_mA - SENSOR_ZERO_CURRENT;
// Все! "Ноль" откалиброван. Теперь твоя программа знает, от какого именно значения тока нужно начинать отсчет.

// Шаг 2: Калибровка "Диапазона" (Масштаба)
// Цель: Убедиться, что если уровень воды 1 метр, программа показывает 1 метр, а не 0.95 или 1.05.

// ДЕЙСТВИЕ: Погрузи датчик на ТОЧНО ИЗВЕСТНУЮ глубину. Проще всего взять ведро, налить воды и линейкой измерить, например, ровно 20 см (0.200 метра).

// НАБЛЮДЕНИЕ: Посмотри в Монитор порта. Программа, скорее всего, покажет немного другое значение, например:
// [Отладка: ... ] Ток: 7.15 мА Уровень воды: 0.198 метров

// ЧТО БРАТЬ И КАК СЧИТАТЬ:

// Мы знаем, что реальная глубина = 0.200 м.
// Программа нам показала измеренную глубину = 0.198 м.
// Теперь мы можем найти "коэффициент вранья" (коэффициент коррекции):
// Коэффициент = Реальное / Измеренное
// Коэффициент = 0.200 / 0.198 ≈ 1.01
// КУДА ВСТАВЛЯТЬ:

// В начало скетча добавь еще одну строку:
// cpp
// #define WATER_LEVEL_CORRECTION_FACTOR 1.01 // Коэффициент коррекции
// В самом конце функции getWaterLevel, перед тем как вернуть значение, найди строку:
// return waterLevel;
// И измени ее, умножив результат на твой коэффициент:
// return waterLevel * WATER_LEVEL_CORRECTION_FACTOR;



