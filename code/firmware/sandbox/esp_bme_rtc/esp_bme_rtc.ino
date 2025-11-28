// int ledPin = LED_BUILTIN;
// int blinkInterval = 250; // Интервал в миллисекундах. 250 = быстрое мигание

// void setup() {
//   pinMode(ledPin, OUTPUT);
// }

// void loop() {
//   digitalWrite(ledPin, HIGH);
//   delay(blinkInterval); // Используем переменную

//   digitalWrite(ledPin, LOW);
//   delay(blinkInterval); // Используем переменную
// }


#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Создаем объекты для каждого сенсора
RTC_DS3231 rtc;
Adafruit_BME280 bme; // I2C

void setup() {
  // Запускаем Serial порт. Для ESP32 можно смело ставить скорость 115200
  Serial.begin(9600);
  while (!Serial) {
    delay(10); // Ожидание для некоторых плат
  }

  Serial.println("Запуск теста BME280 + DS3231...");

  // --- Инициализация DS3231 ---
  if (!rtc.begin()) {
    Serial.println("Не удалось найти RTC модуль DS3231! Проверьте подключение.");
    while (1) delay(10);
  }
  Serial.println("RTC модуль DS3231 найден.");

  // --- БЛОК ДЛЯ УСТАНОВКИ ВРЕМЕНИ (НУЖНО СДЕЛАТЬ ОДИН РАЗ) ---
  // Раскомментируйте следующую строку, чтобы установить время RTC
  // равным времени компиляции этого скетча.
  // После первой успешной прошивки, ЗАКОММЕНТИРУЙТЕ эту строку обратно
  // и прошейте еще раз, чтобы не сбрасывать время при каждой перезагрузке.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));


  // --- Инициализация BME280 ---
  // Адрес по умолчанию 0x77, но может быть 0x76.
  // Если не находит, попробуйте bme.begin(0x76)
  if (!bme.begin(0x76)) {
    Serial.println("Не удалось найти сенсор BME280! Проверьте подключение и I2C адрес.");
    while (1) delay(10);
  }
  Serial.println("Сенсор BME280 найден.");
  Serial.println("---------------------------------------------");
}

void loop() {
  // --- Считываем данные с DS3231 ---
  DateTime now = rtc.now();
  float rtc_temp = rtc.getTemperature();

  // --- Считываем данные с BME280 ---
  float bme_temp = bme.readTemperature();
  float bme_humidity = bme.readHumidity();
  // Давление BME280 выдает в Паскалях. Для удобства переводим в гектопаскали (гПа).
  float bme_pressure = bme.readPressure() / 100.0F;


  // --- Форматируем и выводим данные в монитор порта ---

  // Дата и время
  Serial.print(now.year(), DEC);
  Serial.print('/');
  if (now.month() < 10) Serial.print('0');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  if (now.day() < 10) Serial.print('0');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  if (now.hour() < 10) Serial.print('0');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  if (now.minute() < 10) Serial.print('0');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  if (now.second() < 10) Serial.print('0');
  Serial.print(now.second(), DEC);

  // Показания сенсоров
  Serial.print(" | BME280: ");
  Serial.print(bme_temp);
  Serial.print(" *C, ");
  Serial.print(bme_pressure);
  Serial.print(" hPa, ");
  Serial.print(bme_humidity);
  Serial.print(" %");

  Serial.print(" | DS3231 Temp: ");
  Serial.print(rtc_temp);
  Serial.println(" *C");

  // Ждем одну секунду перед следующим считыванием
  delay(1000);
}
