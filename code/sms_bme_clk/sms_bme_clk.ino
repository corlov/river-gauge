// --- НАСТРОЙКИ ПОЛЬЗОВАТЕЛЯ ---
// Номер телефона для отправки SMS в международном формате
#define SMS_TARGET "+79055191010"

#define SLEEP_DURATION_SECONDS 120 // Время сна в секундах (1 минута)

// --- ПИНЫ ---
#define LED_PIN LED_BUILTIN
#define MODEM_RST_PIN 5
// --- ПРАВИЛЬНЫЕ ПИНЫ ДЛЯ ВАШЕЙ ESP32-S3 ---
#define MODEM_RX_PIN 15 // Подключается к TXD на SIM800
#define MODEM_TX_PIN 16 // Подключается к RXD на SIM800

// --- БИБЛИОТЕКИ ---
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// --- ОБЪЕКТЫ И ПЕРЕМЕННЫЕ ---
RTC_DS3231 rtc;
Adafruit_BME280 bme;

// Используем UART-порт №1 (так как на пинах написано RX1/TX1)
HardwareSerial SerialGSM(1);
TinyGsm modem(SerialGSM);

// Функция для мигания светодиодом
void indicateWakeUp() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(350);
  digitalWrite(LED_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  indicateWakeUp();
  Serial.println("\nПроснулся!");

  // --- 1. Инициализация датчиков ---
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("Ошибка: не найден RTC DS3231!");
    goToSleep();
  }
  if (!bme.begin(0x76)) {
    Serial.println("Ошибка: не найден датчик BME280!");
    goToSleep();
  }

  // =========================================================================
  // Раскомментируйте для первой прошивки, чтобы установить время
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // =========================================================================

  // --- 2. Считываем данные ---
  DateTime now = rtc.now();
  char timestamp[20];
  sprintf(timestamp, "%02d.%02d.%04d %02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute());

  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  Serial.println("--- Данные для отправки ---");
  Serial.print("Время: "); Serial.println(timestamp);
  Serial.print("Температура: "); Serial.println(temperature);
  Serial.print("Влажность: "); Serial.println(humidity);
  Serial.print("Давление: "); Serial.println(pressure);
  Serial.println("---------------------------");

  // --- 3. Инициализация модема и отправка SMS ---
  pinMode(MODEM_RST_PIN, OUTPUT);
  digitalWrite(MODEM_RST_PIN, HIGH);
  // --- ИНИЦИАЛИЗИРУЕМ UART С УКАЗАНИЕМ ПИНОВ ---
  SerialGSM.begin(9600, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

  Serial.println("Инициализация модема...");
  if (!modem.restart()) {
    Serial.println("Не удалось перезагрузить модем!");
    goToSleep();
  }
  Serial.println("Модем готов.");

  Serial.print("Ожидание сети...");
  if (!modem.waitForNetwork()) {
    Serial.println(" сеть не найдена.");
    goToSleep();
  }
  Serial.println(" сеть найдена.");

  // Формируем текст SMS-сообщения
  String smsMessage = "";
  smsMessage += String(timestamp) + ",";
  smsMessage += String(temperature, 1) + ",";
  smsMessage += String(humidity, 1) + ",";
  smsMessage += String(pressure, 1);

  Serial.println("Текст сообщения:");
  Serial.println(smsMessage);

  Serial.print("Отправка SMS на номер ");
  Serial.println(SMS_TARGET);

  bool success = modem.sendSMS(SMS_TARGET, smsMessage);

  if (success) {
    Serial.println("SMS успешно отправлено!");
  } else {
    Serial.println("Не удалось отправить SMS.");
  }

  // --- 4. Уходим в сон ---
  goToSleep();
}

void goToSleep() {
  Serial.print("Ухожу в сон на ");
  Serial.print(SLEEP_DURATION_SECONDS);
  Serial.println(" секунд...");
  Serial.flush();
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION_SECONDS * 1000000);
  esp_deep_sleep_start();
}

void loop() {
  // Не используется
}


// // --- НАСТРОЙКИ ПОЛЬЗОВАТЕЛЯ ---
// // Номер телефона для отправки SMS в международном формате
// #define SMS_TARGET "+79055191010"

// #define SLEEP_DURATION_SECONDS 60 // Время сна в секундах (1 минута)

// // --- ПИНЫ ---
// #define LED_PIN LED_BUILTIN
// #define MODEM_RST_PIN 5

// // --- БИБЛИОТЕКИ ---
// #define TINY_GSM_MODEM_SIM800
// #include <TinyGsmClient.h> // Оставляем, т.к. TinyGsm.h зависит от него
// #include <Wire.h>
// #include "RTClib.h"
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME280.h>

// // --- ОБЪЕКТЫ И ПЕРЕМЕННЫЕ ---
// RTC_DS3231 rtc;
// Adafruit_BME280 bme;

// HardwareSerial SerialGSM(2); // UART для модема (GPIO 16, 17)
// TinyGsm modem(SerialGSM);

// // Функция для мигания светодиодом
// void indicateWakeUp() {
//   pinMode(LED_PIN, OUTPUT);
//   digitalWrite(LED_PIN, HIGH);
//   delay(150);
//   digitalWrite(LED_PIN, LOW);
// }

// void setup() {
//   Serial.begin(115200);
//   delay(1000);

//   indicateWakeUp();
//   Serial.println("\nПроснулся!");

//   // --- 1. Инициализация датчиков ---
//   Wire.begin();
//   if (!rtc.begin()) {
//     Serial.println("Ошибка: не найден RTC DS3231!");
//     goToSleep();
//   }
//   if (!bme.begin(0x76)) {
//     Serial.println("Ошибка: не найден датчик BME280!");
//     goToSleep();
//   }

//   // =========================================================================
//   // Раскомментируйте для первой прошивки, чтобы установить время
//   // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//   // =========================================================================

//   // --- 2. Считываем данные ---
//   DateTime now = rtc.now();
//   char timestamp[20];
//   sprintf(timestamp, "%02d.%02d.%04d %02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute());

//   float temperature = bme.readTemperature();
//   float humidity = bme.readHumidity();
//   float pressure = bme.readPressure() / 100.0F;

//   Serial.println("--- Данные для отправки ---");
//   Serial.print("Время: "); Serial.println(timestamp);
//   Serial.print("Температура: "); Serial.println(temperature);
//   Serial.print("Влажность: "); Serial.println(humidity);
//   Serial.print("Давление: "); Serial.println(pressure);
//   Serial.println("---------------------------");

//   // --- 3. Инициализация модема и отправка SMS ---
//   pinMode(MODEM_RST_PIN, OUTPUT);
//   digitalWrite(MODEM_RST_PIN, HIGH);
//   SerialGSM.begin(9600);

//   Serial.println("Инициализация модема...");
//   if (!modem.restart()) {
//     Serial.println("Не удалось перезагрузить модем!");
//     goToSleep();
//   }
//   Serial.println("Модем готов.");

//   Serial.print("Ожидание сети...");
//   if (!modem.waitForNetwork()) {
//     Serial.println(" сеть не найдена.");
//     goToSleep();
//   }
//   Serial.println(" сеть найдена.");

//   // Формируем текст SMS-сообщения
//   String smsMessage = "Датчик:\n";
//   smsMessage += String(timestamp) + ",";
//   smsMessage += "T: " + String(temperature, 1) + " C,"; // 1 знак после запятой
//   smsMessage += "H: " + String(humidity, 1) + " %,";
//   smsMessage += "P: " + String(pressure, 1) + " hPa";

//   Serial.println("Текст сообщения:");
//   Serial.println(smsMessage);

//   Serial.print("Отправка SMS на номер ");
//   Serial.println(SMS_TARGET);

//   // Отправляем SMS
//   bool success = modem.sendSMS(SMS_TARGET, smsMessage);

//   if (success) {
//     Serial.println("SMS успешно отправлено!");
//   } else {
//     Serial.println("Не удалось отправить SMS.");
//   }

//   // --- 4. Уходим в сон ---
//   goToSleep();
// }

// void goToSleep() {
//   Serial.print("Ухожу в сон на ");
//   Serial.print(SLEEP_DURATION_SECONDS);
//   Serial.println(" секунд...");
//   Serial.flush();
//   esp_sleep_enable_timer_wakeup(SLEEP_DURATION_SECONDS * 1000000);
//   esp_deep_sleep_start();
// }

// void loop() {
//   // Не используется
// }
