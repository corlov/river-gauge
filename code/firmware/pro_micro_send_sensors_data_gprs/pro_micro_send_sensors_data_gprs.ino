#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER   64
#define TINY_GSM_HAS_SMS      0
#define TINY_GSM_HAS_CALLS    0
#define TINY_GSM_HAS_LOCATION 0
#define TINY_GSM_HAS_FTP      0
#define TINY_GSM_HAS_BLUETOOTH 0
#define TINY_GSM_HAS_MQTT     0
#define TINY_GSM_HAS_SSL      0

#define SENSORS 1

#include <TinyGsmClient.h>
#include <Wire.h>
#include "RTClib.h"
//#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

const char apn[]    = "internet";
const char server[] = "94.228.123.208";
const int  port     = 8001;

RTC_DS3231 rtc;
Adafruit_BME280 bme;

#define SerialGSM Serial1
TinyGsm modem(SerialGSM);
TinyGsmClient client(modem);



#define LED_PIN         17
#define MODEM_RST_PIN   6
#define MODEM_POWER_PIN 5



void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(MODEM_POWER_PIN, OUTPUT);

  digitalWrite(MODEM_POWER_PIN, HIGH);

  // Запускаем Serial для отладки в мониторе порта
  Serial.begin(9600);
  delay(3000); // Даем время на открытие монитора порта

  #ifdef SENSORS

    digitalWrite(MODEM_POWER_PIN, LOW);

    digitalWrite(LED_PIN, HIGH);
    Wire.begin(); // На Pro Micro это пины 2(SDA) и 3(SCL)
    
    if (!rtc.begin()) {
      Serial.println(F("E1"));
    }

    if (!bme.begin(0x76)) {
      Serial.println(F("E2"));    
    }
  #endif

  SerialGSM.begin(9600);

  //Serial.println(F("Initializing modem..."));
  if (!modem.restart()) {
    Serial.println(F("Failed"));
    while (true); 
  }
  Serial.println(F("ready"));
  
  if (!modem.waitForNetwork()) {
    Serial.println(F("fail."));
    while (true);
  }
  
  Serial.print(F("Connecting"));
  if (!modem.gprsConnect(apn)) {
    Serial.println(F("fail."));
    while (true);
  }
  
  Serial.println(F("Setup ok"));
}



void loop() {
  Serial.print(server);
  Serial.print(F(":"));
  Serial.println(port);


  char postData[30];
  #ifdef SENSORS
    DateTime now = rtc.now();
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;

    char tempStr[8];
    char humStr[8];
    char pressStr[8];

    // Преобразуем каждое float-число в строку
    dtostrf(temperature, 4, 1, tempStr); // 4 - общая ширина, 1 - знак после запятой
    dtostrf(humidity, 4, 1, humStr);
    dtostrf(pressure, 6, 1, pressStr);   // Давление может быть > 1000, берем ширину побольше

    snprintf(postData, sizeof(postData), "%04d-%02d-%02d %02d:%02d,%s,%s,%s", now.year(), now.month(), now.day(), now.hour(), now.minute(), tempStr, humStr, pressStr);
  #else
  
  #endif

  // Устанавливаем соединение с сервером
  if (client.connect(server, port)) {
    client.print(postData);
    Serial.println(postData);
    client.stop();
  } 
  else {
    Serial.println(F("failed."));
  }

  // Отключаемся от GPRS для чистоты эксперимента
  modem.gprsDisconnect();
  
  digitalWrite(MODEM_POWER_PIN, HIGH);
  delay(5000);
  digitalWrite(MODEM_POWER_PIN, LOW);
}
