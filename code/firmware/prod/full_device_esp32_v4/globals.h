#ifndef GLOBALS_H_
#define GLOBALS_H_

#define TINY_GSM_MODEM_SIM800

#include "RTClib.h"
#include <Adafruit_BME280.h>
#include <TinyGsmClient.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_INA219.h>
#include <PubSubClient.h>


extern RTC_DS3231 rtc;

extern Adafruit_BME280 bme;

extern HardwareSerial SerialGSM;

extern TinyGsm modem;

extern TinyGsmClient client;

extern OneWire oneWire;

extern DallasTemperature sensors;

extern Preferences preferences;

extern Adafruit_ADS1115 ads;

extern Adafruit_INA219 ina219;

// флаг что мы получили ответ от Брокера
extern bool settingsReceived;

extern PubSubClient mqttClient;

#endif
