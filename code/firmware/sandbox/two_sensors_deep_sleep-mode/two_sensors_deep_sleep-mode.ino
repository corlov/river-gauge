#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <MS5837.h>
#include "RTClib.h" // <-- Библиотека для часов
#include "FS.h"     // <-- Библиотека для файловой системы
#include "SPIFFS.h" // <-- Драйвер для SPIFFS

// --- Настройки ---

// Пины для стандартной шины I2C (BME280 и DS3231)
#define I2C0_SDA 21
#define I2C0_SCL 22

// Пины для второй шины I2C (для MS5837)
#define I2C1_SDA 8
#define I2C1_SCL 9

// Настройки глубокого сна
#define uS_TO_S_FACTOR 1000000ULL  // Фактор конвертации из микросекунд в секунды
#define TIME_TO_SLEEP  3600        // Время сна в секундах (3600 = 1 час)

// Имя файла для логов
#define LOG_FILE "/water_level_log.csv"

// Константы для расчета
const float DENSITY_OF_FRESH_WATER = 997.0;
const float GRAVITY_ACCELERATION = 9.80665;

// --- Объекты ---
Adafruit_BME280 bme;
MS5837 ms5837;
RTC_DS3231 rtc; // <-- Объект для часов реального времени

// --- Прототип функции ---
void logDataToFile(String data);

void setup() {
  Serial.begin(115200);
  delay(2000); // Даем время на открытие Serial монитора
  Serial.println("\n--- Автономный логгер уровня воды ---");

  // --- Инициализация файловой системы SPIFFS ---
  Serial.println("Инициализация SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("Ошибка монтирования SPIFFS! Устройство перезагрузится.");
    ESP.restart();
  }
  Serial.println("SPIFFS смонтирована.");

  // --- Инициализация BME280 и RTC на первой шине I2C ---
  Wire.begin(I2C0_SDA, I2C0_SCL);

  Serial.println("Инициализация BME280...");
  if (!bme.begin(0x76, &Wire)) {
    Serial.println("Не удалось найти датчик BME280!");
    // В автономном режиме лучше не останавливаться, а просто записать ошибку
  } else {
    Serial.println("BME280 найден!");
  }

  Serial.println("Инициализация RTC DS3231...");
  if (!rtc.begin()) {
    Serial.println("Не удалось найти RTC!");
  } else {
    Serial.println("RTC найден!");
    // Следующие строки нужны для ОДНОКРАТНОЙ установки времени.
    // 1. Раскомментируйте их.
    // 2. Загрузите скетч.
    // 3. Снова закомментируйте и загрузите еще раз.
    // Иначе время будет сбрасываться при каждой перезагрузке.
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // Serial.println("Время RTC установлено по времени компиляции.");
  }

  // --- Инициализация MS5837 на второй шине I2C ---
  Wire.begin(I2C1_SDA, I2C1_SCL);
  Serial.println("Инициализация MS5837...");
  if (!ms5837.init()) {
      Serial.println("Ошибка инициализации MS5837!");
  } else {
    ms5837.setModel(MS5837::MS5837_30BA);
    ms5837.setFluidDensity(DENSITY_OF_FRESH_WATER);
    Serial.println("MS5837 готов к работе!");
  }

  // --- Основная логика, которая выполняется один раз при пробуждении ---

  // 1. Получаем текущее время
  DateTime now = rtc.now();
  char timestamp[25];
  sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  Serial.print("Текущее время: ");
  Serial.println(timestamp);

  // 2. Чтение данных с BME280
  Wire.begin(I2C0_SDA, I2C0_SCL);
  float pressure_atmosphere_mbar = bme.readPressure() / 100.0F;

  // 3. Чтение данных с MS5837
  Wire.begin(I2C1_SDA, I2C1_SCL);
  ms5837.read();
  float pressure_total_mbar = ms5837.pressure();
  float water_temp = ms5837.temperature();

  // 4. Расчеты
  float water_depth_cm = -1.0; // Значение по умолчанию в случае ошибки
  if (!isnan(pressure_total_mbar) && !isnan(pressure_atmosphere_mbar)) {
    float pressure_water_mbar = pressure_total_mbar - pressure_atmosphere_mbar;
    if (pressure_water_mbar < 0) {
      pressure_water_mbar = 0;
    }
    float pressure_water_pa = pressure_water_mbar * 100.0;
    water_depth_cm = (pressure_water_pa / (DENSITY_OF_FRESH_WATER * GRAVITY_ACCELERATION)) * 100.0;
  } else {
    Serial.println("Ошибка чтения данных с одного из датчиков!");
  }

  // 5. Формирование строки для записи в файл
  String dataString = String(timestamp) + "," +
                      String(water_depth_cm, 2) + "," +
                      String(water_temp, 2) + "," +
                      String(pressure_atmosphere_mbar, 2) + "," +
                      String(pressure_total_mbar, 2);

  Serial.print("Строка для записи: ");
  Serial.println(dataString);

  // 6. Запись данных в файл
  logDataToFile(dataString);

  // 7. Подготовка и уход в глубокий сон
  Serial.printf("Ухожу в сон на %d секунд...\n", TIME_TO_SLEEP);
  Serial.flush(); // Обязательно, чтобы все сообщения успели отправиться
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {
  // Этот код никогда не будет выполнен, так как ESP32 засыпает в конце setup()
}

// Функция для записи данных в файл на SPIFFS
void logDataToFile(String data) {
  Serial.printf("Запись в файл %s\n", LOG_FILE);

  // Открываем файл в режиме добавления (append)
  File file = SPIFFS.open(LOG_FILE, FILE_APPEND);
  if (!file) {
    Serial.println("Не удалось открыть файл для записи");
    // Попробуем создать файл с заголовком
    file = SPIFFS.open(LOG_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("Не удалось создать файл!");
        return;
    }
  }

  // Если файл пустой (только что создан), добавляем заголовок
  if (file.size() == 0) {
    Serial.println("Файл пуст, добавляю заголовок.");
    file.println("Timestamp,Water_Depth_cm,Water_Temp_C,Atm_Pressure_mbar,Total_Pressure_mbar");
  }

  // Записываем строку с данными
  if (file.println(data)) {
    Serial.println("Данные успешно записаны.");
  } else {
    Serial.println("Ошибка записи данных.");
  }
  file.close();
}
