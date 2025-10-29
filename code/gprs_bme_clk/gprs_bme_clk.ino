// --- НАСТРОЙКИ ПОЛЬЗОВАТЕЛЯ ---
// УДАЛЕНО: #define SMS_TARGET "+79055191010"

// --- НОВЫЕ НАСТРОЙКИ СЕРВЕРА И GPRS ---
const char apn[]      = "internet"; // APN вашего мобильного оператора (например, "internet.mts.ru", "internet.beeline.ru", "internet.tele2.ru")
const char gprsUser[] = "";         // Логин для APN (обычно не нужен)
const char gprsPass[] = "";         // Пароль для APN (обычно не нужен)

const char server[] = "94.228.123.208"; // <<< ЗАМЕНИТЕ НА IP-АДРЕС ВАШЕГО КОМПЬЮТЕРА
const int  port     = 8001;            // Порт, который слушает Python-скрипт

#define SLEEP_DURATION_SECONDS 30 // Время сна в секундах

// --- ПИНЫ ---
#define LED_PIN LED_BUILTIN
#define MODEM_RST_PIN 5
#define MODEM_RX_PIN 15
#define MODEM_TX_PIN 16

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

HardwareSerial SerialGSM(1);
TinyGsm modem(SerialGSM);
TinyGsmClient client(modem); // <<< Клиент для отправки данных через интернет

// Функция для мигания светодиодом
void indicateWakeUp() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(350);
  digitalWrite(LED_PIN, LOW);
  delay(350);
  digitalWrite(LED_PIN, HIGH);
  delay(350);
  digitalWrite(LED_PIN, LOW);
  delay(350);
  digitalWrite(LED_PIN, HIGH);
  delay(350);
  digitalWrite(LED_PIN, LOW);
  delay(350);
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

  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // --- 2. Считываем данные ---
  DateTime now = rtc.now();
  char timestamp[20];
  sprintf(timestamp, "%02d.%02d.%04d %02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute());

  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  // Формируем тело сообщения для отправки на сервер
  String postData = "";
  postData += String(timestamp) + ",";
  postData += String(temperature, 1) + ",";
  postData += String(humidity, 1) + ",";
  postData += String(pressure, 1);

  Serial.println("--- Данные для отправки ---");
  Serial.println(postData);
  Serial.println("---------------------------");

  // --- 3. Инициализация модема и отправка данных ---
  pinMode(MODEM_RST_PIN, OUTPUT);
  digitalWrite(MODEM_RST_PIN, HIGH);
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

  Serial.print("Подключение к GPRS...");
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(" не удалось подключиться.");
    goToSleep();
  }
  Serial.println(" GPRS подключен.");

  // --- ОТПРАВКА ДАННЫХ НА СЕРВЕР ВМЕСТО SMS ---
  Serial.print("Отправка данных на сервер ");
  Serial.print(server);
  Serial.print(":");
  Serial.println(port);

  if (client.connect(server, port)) {
    Serial.println("Соединение с сервером установлено.");

    // Формируем и отправляем HTTP POST-запрос
    client.print(String("POST / HTTP/1.1\r\n"));
    client.print(String("Host: ") + server + "\r\n");
    client.print("Connection: close\r\n");
    client.print("Content-Type: text/plain\r\n");
    client.print(String("Content-Length: ") + postData.length() + "\r\n");
    client.print("\r\n"); // Пустая строка - обязательный разделитель
    client.print(postData);

    Serial.println("Данные отправлены.");

    // Ждем ответа от сервера (необязательно, но полезно для отладки)
    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 5000L) {
      while (client.available()) {
        char c = client.read();
        Serial.print(c);
        timeout = millis();
      }
    }
    Serial.println();

    client.stop();
    Serial.println("Соединение закрыто.");
  } else {
    Serial.println("Не удалось подключиться к серверу.");
  }

  modem.gprsDisconnect();
  Serial.println("GPRS отключен.");
  // --- КОНЕЦ БЛОКА ОТПРАВКИ ДАННЫХ ---

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
