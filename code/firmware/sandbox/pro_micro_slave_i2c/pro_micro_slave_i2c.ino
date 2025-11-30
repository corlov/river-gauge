// --- ПРОШИВКА ДЛЯ СЕНСОРНОГО МОДУЛЯ (I2C + DS18B20) ---
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define SLAVE_ADDRESS 8

// --- НАСТРОЙКИ ПИНОВ ---
#define WATER_LEVEL_PIN A0
#define BATTERY_VOLTAGE_PIN A1
#define WATER_TEMPERATURE_PIN 4 // Используем цифровой пин 4 для DS18B20

// --- НАСТРОЙКИ ДАТЧИКОВ ---
#define SAMPLES_COUNT 15
// ... все остальные #define для расчетов (SHUNT_RESISTOR, R1, R2 и т.д.) ...
#define SHUNT_RESISTOR 150.0
#define SENSOR_RANGE_METERS 5.0
#define R1 47000.0
#define R2 12000.0
#define V_REF 5.0
#define ADC_RESOLUTION 1023.0

// --- Инициализация объектов для DS18B20 ---
OneWire oneWire(WATER_TEMPERATURE_PIN);
DallasTemperature sensors(&oneWire);

volatile String dataToSend = "0.0,0.0,0.0"; // Теперь у нас три значения

void setup() {
  Wire.begin(SLAVE_ADDRESS);
  Wire.onRequest(requestEvent);
  sensors.begin(); // Запускаем датчик температуры


  Serial.begin(9600);
  pinMode(17, OUTPUT);
}

void loop() {
  // Проводим все три измерения
  float waterLevel = getWaterLevel();
  float batteryVoltage = getBatteryVoltage();
  float waterTemp = getWaterTemperature();

  // Формируем новую строку ответа: "уровень,напряжение,температура"
  dataToSend = String(waterLevel, 2) + "," + String(batteryVoltage, 2) + "," + String(waterTemp, 2);

  Serial.print(String(waterLevel, 2) + "," + String(batteryVoltage, 2) + "," + String(waterTemp, 2) + "\n");
  digitalWrite(17, HIGH);
  delay(100); // Уменьшим задержку, чтобы быстрее увидеть результат
  digitalWrite(17, LOW);
  delay(100);
}

void requestEvent() {
  Wire.write(dataToSend.c_str());
}





// --- НОВАЯ ФУНКЦИЯ ДЛЯ ЧТЕНИЯ ТЕМПЕРАТУРЫ ---
float getWaterTemperature() {
  sensors.requestTemperatures(); // Отправляем команду на измерение
  float tempC = sensors.getTempCByIndex(0); // Считываем температуру с первого датчика на шине

  // Проверка на ошибку чтения (-127 - это код ошибки датчика)
  if (tempC == DEVICE_DISCONNECTED_C) {
    return -999.0; // Возвращаем специальное значение, если датчик не найден
  }
  return tempC;
}






// --- ФУНКЦИЯ ДЛЯ ДАТЧИКА УРОВНЯ ВОДЫ С МЕДИАННЫМ ФИЛЬТРОМ ---
float getWaterLevel() {
  int samples[SAMPLES_COUNT];

  // Шаг 1: Собираем N сэмплов
  for (int i = 0; i < SAMPLES_COUNT; i++) {
    samples[i] = analogRead(WATER_LEVEL_PIN);
    delay(10); // Небольшая пауза между измерениями
  }

  // Шаг 2: Сортируем массив (простой пузырьковой сортировкой)
  for (int i = 0; i < SAMPLES_COUNT - 1; i++) {
    for (int j = 0; j < SAMPLES_COUNT - i - 1; j++) {
      if (samples[j] > samples[j + 1]) {
        int temp = samples[j];
        samples[j] = samples[j + 1];
        samples[j + 1] = temp;
      }
    }
  }

  // Шаг 3: Берем средний элемент (медиану)
  int medianRawValue = samples[SAMPLES_COUNT / 2];

  // Шаг 4: Проводим расчеты на основе "очищенного" значения
  float voltage = (medianRawValue / ADC_RESOLUTION) * V_REF;
  float current_mA = (voltage / SHUNT_RESISTOR) * 1000.0;
  float useful_current = current_mA - 4.0;
  const float current_range = 16.0;
  float waterLevel = (useful_current / current_range) * SENSOR_RANGE_METERS;
  if (waterLevel < 0) waterLevel = 0.0;
  return waterLevel;
}




// --- ФУНКЦИЯ ДЛЯ ДАТЧИКА НАПРЯЖЕНИЯ АКБ С МЕДИАННЫМ ФИЛЬТРОМ ---
float getBatteryVoltage() {
  int samples[SAMPLES_COUNT];

  // Шаг 1: Собираем N сэмплов
  for (int i = 0; i < SAMPLES_COUNT; i++) {
    samples[i] = analogRead(BATTERY_VOLTAGE_PIN);
    delay(10);
  }

  // Шаг 2: Сортируем массив
  for (int i = 0; i < SAMPLES_COUNT - 1; i++) {
    for (int j = 0; j < SAMPLES_COUNT - i - 1; j++) {
      if (samples[j] > samples[j + 1]) {
        int temp = samples[j];
        samples[j] = samples[j + 1];
        samples[j + 1] = temp;
      }
    }
  }

  // Шаг 3: Берем медиану
  int medianRawValue = samples[SAMPLES_COUNT / 2];

  // Шаг 4: Проводим расчеты
  float vout = (medianRawValue / ADC_RESOLUTION) * V_REF;
  float vin = vout * (R1 + R2) / R2;
  return vin;
}


