// --- НАСТРОЙКИ ДЛЯ УМЕНЬШЕНИЯ РАЗМЕРА ПРОШИВКИ ---
// Этот блок должен быть в самом начале файла, до #include
#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER   64
#define TINY_GSM_HAS_SMS      0
#define TINY_GSM_HAS_CALLS    0
#define TINY_GSM_HAS_LOCATION 0
#define TINY_GSM_HAS_FTP      0
#define TINY_GSM_HAS_BLUETOOTH 0
#define TINY_GSM_HAS_MQTT     0
#define TINY_GSM_HAS_SSL      0
// --- КОНЕЦ БЛОКА НАСТРОЕК ---

#include <TinyGsmClient.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// --- НАСТРОЙКИ СЕТИ И СЕРВЕРА ---
const char apn[]    = "internet"; // APN твоего оператора
const char server[] = "94.228.123.208";
const int  port     = 8001;


// --- ОБЪЕКТЫ И ПЕРЕМЕННЫЕ ---
RTC_DS3231 rtc;
Adafruit_BME280 bme;


// --- ПИНЫ ---
// Мы не используем RST в этом простом примере, чтобы сэкономить место

// --- ОБЪЕКТЫ МОДЕМА ---
// Используем аппаратный Serial1 на пинах 0(RX) и 1(TX)
#define SerialGSM Serial1
TinyGsm modem(SerialGSM);
TinyGsmClient client(modem);


// --- ПИНЫ ДЛЯ PRO MICRO ---
#define LED_PIN       17  // <<< ИЗМЕНЕНО: Используем RX LED на пине 17
#define MODEM_RST_PIN 5


void blink() {
  int i = 0;
  while(i++ < 3) {
    digitalWrite(17, HIGH);
    delay(700); // Уменьшим задержку, чтобы быстрее увидеть результат
    digitalWrite(17, LOW);
    delay(200);
  }
}




void setup() {
  // Запускаем Serial для отладки в мониторе порта
  Serial.begin(9600);
  delay(3000); // Даем время на открытие монитора порта

  pinMode(LED_PIN, OUTPUT);
  blink();
  Wire.begin(); // На Pro Micro это пины 2(SDA) и 3(SCL)
  
  if (!rtc.begin()) {
    Serial.println(F("Error: 1"));
  }

  if (!bme.begin(0x76)) {
    Serial.println(F("Error: 2"));    
  }



  Serial.println(F("--- GSM Test Message Sender ---"));

  // <<< ВАЖНО! Установи здесь ту скорость, на которой модем ответил OK!
  // <<< Возможно, это 115200 или 57600, а не 9600.
  SerialGSM.begin(9600);

  Serial.println(F("Initializing modem..."));
  if (!modem.restart()) {
    Serial.println(F("Failed to restart modem. Halting."));
    while (true); // Остановка программы
  }
  Serial.println(F("Modem is ready."));

  Serial.print(F("Waiting for network..."));
  if (!modem.waitForNetwork()) {
    Serial.println(F(" fail."));
    while (true);
  }
  Serial.println(F(" OK."));

  Serial.print(F("Connecting to GPRS..."));
  if (!modem.gprsConnect(apn)) {
    Serial.println(F(" fail."));
    while (true);
  }
  Serial.println(F(" OK."));

  Serial.println(F("\nSetup complete. Ready to send message."));
}

void loop() {
  Serial.print(F("Connecting to server "));
  Serial.print(server);
  Serial.print(F(":"));
  Serial.println(port);


 // --- 1. Считываем данные с датчиков ---
  DateTime now = rtc.now();
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  char postData[128];
  char tempStr[8];
  char humStr[8];
  char pressStr[8];

  // Преобразуем каждое float-число в строку
  dtostrf(temperature, 4, 1, tempStr); // 4 - общая ширина, 1 - знак после запятой
  dtostrf(humidity, 4, 1, humStr);
  dtostrf(pressure, 6, 1, pressStr);   // Давление может быть > 1000, берем ширину побольше

  snprintf(postData, sizeof(postData), "%04d-%02d-%02d %02d:%02d,%s,%s,%s", now.year(), now.month(), now.day(), now.hour(), now.minute(), tempStr, humStr, pressStr);


  // Устанавливаем соединение с сервером
  if (client.connect(server, port)) {
    Serial.println(F("Connection successful!"));

    // --- Отправляем наше тестовое сообщение ---
    client.print(postData);
    // -----------------------------------------

    Serial.print(F("Message sent: "));
    Serial.println(postData);

    client.stop();
    Serial.println(F("Connection closed."));
  } else {
    Serial.println(F("Connection failed."));
  }

  // Отключаемся от GPRS для чистоты эксперимента
  modem.gprsDisconnect();
  Serial.println(F("GPRS disconnected."));

  Serial.println(F("\nTest complete. Halting program."));
  // Бесконечный цикл, чтобы программа остановилась после одной отправки
  while (true) {
    delay(1000);
  }
}
