#ifndef GLOBALS_H_
#define GLOBALS_H_


#define TINY_GSM_MODEM_SIM800

#include "RTClib.h"
#include <Adafruit_BME280.h>
#include <TinyGsmClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>



extern RTC_DS3231 rtc;
extern Adafruit_BME280 bme;

extern HardwareSerial SerialGSM;
extern TinyGsm modem;
extern TinyGsmClient client;

extern OneWire oneWire;
extern DallasTemperature sensors;

extern Adafruit_NeoPixel pixels;

extern Preferences preferences;


#endif
