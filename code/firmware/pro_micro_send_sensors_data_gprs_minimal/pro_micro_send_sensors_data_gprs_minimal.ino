#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER   64
#define TINY_GSM_HAS_SMS      0
#define TINY_GSM_HAS_CALLS    0
#define TINY_GSM_HAS_LOCATION 0
#define TINY_GSM_HAS_FTP      0
#define TINY_GSM_HAS_BLUETOOTH 0
#define TINY_GSM_HAS_MQTT     0
#define TINY_GSM_HAS_SSL      0

#include <TinyGsmClient.h>
#include <Wire.h>
#include "RTClib.h"
#include <SparkFunBME280.h>

BME280 bme; 
RTC_DS3231 rtc;

#define DEVICE_ID 7001

const char apn[]    = "internet";
const char server[] = "94.228.123.208";
const int  port     = 8001;

#define SerialGSM Serial1
TinyGsm modem(SerialGSM);
TinyGsmClient client(modem);

#define LED_PIN         17
#define MODEM_RST_PIN   6
#define MODEM_POWER_PIN 5



void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(MODEM_POWER_PIN, OUTPUT);

  digitalWrite(MODEM_POWER_PIN, LOW);

  Serial.begin(9600);
  delay(3000); 
  
  digitalWrite(MODEM_POWER_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);

  // На Pro Micro это пины 2(SDA) и 3(SCL)
  Wire.begin();
  
  if (!rtc.begin()) {
    Serial.println(F("E1"));
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  bme.setI2CAddress(0x76);
  if (!bme.beginI2C()) { 
    Serial.println(F("E2")); 
  }
}



void initNetwork() {
  SerialGSM.begin(9600);
  Serial.println(F("Initializing modem..."));
  if (!modem.restart()) {
    Serial.println(F("Failed"));
    while (true); 
  }
  Serial.println(F("Ready"));
  
  if (!modem.waitForNetwork()) {
    Serial.println(F("fail."));
    while (true);
  }
  Serial.println(F("NET OK"));

  if (!modem.gprsConnect(apn)) {
    Serial.println(F("fail."));
    while (true);
  }  
  Serial.println(F("GPRS OK"));
}


void formatPostData(char* buffer, size_t bufferSize) {
  // 1. Считываем данные
  DateTime now = rtc.now();
  float temperature = bme.readTempC();
  float humidity = bme.readFloatHumidity();
  float pressure = bme.readFloatPressure() / 100.0F;

  // 2. Преобразуем float в целые числа (long), умножив на 10,
  // чтобы сохранить один знак после запятой.
  long temp_int = temperature * 10;
  long hum_int = humidity * 10;
  long press_int = pressure * 10;

  // 3. Формируем строку за один вызов, используя целочисленные типы.
  // snprintf_P читает строку формата (PSTR) из Flash-памяти, экономя SRAM.
  snprintf_P(buffer, bufferSize,
             PSTR("%04d,%04d-%02d-%02d %02d:%02d,%ld.%1ld,%ld.%1ld,%ld.%1ld\n"),
             DEVICE_ID,
             now.year(), now.month(), now.day(), now.hour(), now.minute(),
             temp_int / 10, abs(temp_int % 10),   // Целая и дробная часть температуры
             hum_int / 10, abs(hum_int % 10),     // Целая и дробная часть влажности
             press_int / 10, abs(press_int % 10)  // Целая и дробная часть давления
  );
}


void loop() {
  digitalWrite(MODEM_POWER_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);  
  initNetwork();  

  char postData[50];
  formatPostData(postData, sizeof(postData));
  Serial.println(postData);

  if (client.connect(server, port)) {
    client.print(postData);    
    client.stop();
  } 
  else {
    Serial.println(F("E3"));
  }

  modem.gprsDisconnect();
  
  digitalWrite(MODEM_POWER_PIN, LOW);
  digitalWrite(LED_PIN, HIGH);
  Serial.println(F("Loop end"));
  delay(20 * 1000);
}


