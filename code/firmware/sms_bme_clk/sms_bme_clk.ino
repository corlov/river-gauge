#include <TinyGsmClient.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SMS_TARGET "+79055191010"
#define SLEEP_DURATION_SECONDS 120
#define LED_PIN LED_BUILTIN
#define MODEM_RST_PIN 5
#define MODEM_RX_PIN 15 // Подключается к TXD на SIM800
#define MODEM_TX_PIN 16 // Подключается к RXD на SIM800
#define TINY_GSM_MODEM_SIM800

RTC_DS3231 rtc;
Adafruit_BME280 bme;

// Используем UART-порт №1 (так как на пинах написано RX1/TX1)
HardwareSerial SerialGSM(1);
TinyGsm modem(SerialGSM);



void indicateWakeUp() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(200);
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
}



void goToSleep() {
  Serial.flush();
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION_SECONDS * 1000000);
  esp_deep_sleep_start();
}



void initEnv() {
  Serial.begin(115200);
  delay(1000);

  indicateWakeUp();
  Serial.println("\nWake up");

  // --- 1. Инициализация датчиков ---
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("error: RTC DS3231!");
    goToSleep();
  }
  if (!bme.begin(0x76)) {
    Serial.println("error: BME280!");
    goToSleep();
  }
}



void initModem() {
    // --- 3. Инициализация модема и отправка SMS ---
  pinMode(MODEM_RST_PIN, OUTPUT);
  digitalWrite(MODEM_RST_PIN, HIGH);
  // --- ИНИЦИАЛИЗИРУЕМ UART С УКАЗАНИЕМ ПИНОВ ---
  SerialGSM.begin(9600, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

  Serial.println("init modem...");
  if (!modem.restart()) {
    Serial.println("reboot modem failed!");
    goToSleep();
  }

  if (!modem.waitForNetwork()) {
    Serial.println("Network err");
    goToSleep();

  }

  // Раскомментируй для первой прошивки, чтобы установить время
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}



String getNextMessage() {
  // Считываем данные ---
  DateTime now = rtc.now();
  char timestamp[20];
  sprintf(timestamp, "%02d.%02d.%04d %02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute());

  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  // Формируем текст SMS-сообщения
  String smsMessage = "";
  smsMessage += String(timestamp) + ",";
  smsMessage += String(temperature, 1) + ",";
  smsMessage += String(humidity, 1) + ",";
  smsMessage += String(pressure, 1);  
  Serial.println(smsMessage);

  return smsMessage;
}



void init() {
  initEnv();
  initModem();
}


void sendMessage() {
  bool success = modem.sendSMS(SMS_TARGET, getNextMessage());

  if (success) {
    Serial.println("SMS ok");
  } else {
    Serial.println("SMS sending error");
  }
}



void setup() {
  init();
  sendMessage();  
  goToSleep();
}



void loop() {

}

