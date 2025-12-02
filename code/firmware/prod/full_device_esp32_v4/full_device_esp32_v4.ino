#include "water_lvl_types.h"
#include "water_lvl_settings.h"
#include "water_lvl_init.h"
#include "globals.h"
#include "storage.h"
#include "sensors.h"
#include "send_data.h"



RTC_DS3231 rtc;

Adafruit_BME280 bme;

HardwareSerial SerialGSM(1);

TinyGsm modem(SerialGSM);

TinyGsmClient client(modem);

OneWire oneWire(WATER_TEMPERATURE_SENSOR_PIN);

DallasTemperature sensors(&oneWire);

Preferences preferences;

Adafruit_ADS1115 ads;

Adafruit_INA219 ina219;

//#define DEBUG_MODE 1



void setup() {
  // #ifdef DEBUG_MODE
  //   init();
  //   modemOn();
  //   return;
  // #endif

  init();

  PrevState prevState = loadAndIncrementBootState();
  

  bool needToRun = ((prevState.bootCount - 1) % readIntSetting(SETTING_ACTIVATION_FREQ, DEFAULT_MODEM_ACTIVATION_FREQENCY) == 0) || (!prevState.success);
  if (needToRun) {
    setSuccess(false);

    modemOn();

    String message = String(prevState.bootCount) + "," + getAlwaysOnSensorsData() + getPowerControlledSensorsData();

    addCsvLine(message);

    if (attemptToSend(message, prevState.failCounter)) {
      setSuccess(true);
      indicationSuccess();
    } else {
      indicationFail();
    }

    digitalWrite(ERRORE_LED_PIN, LOW);
    modemOff();
  }
  else {
    addCsvLine(String(prevState.bootCount) + "," + getAlwaysOnSensorsData());
    indicationSuccessWithoutSend();
  }

  powerOff();
}



void loop() {
  // #ifdef DEBUG_MODE
  //   String defaultSensorsData = getAlwaysOnSensorsData();
  //   String fullMessage = defaultSensorsData + getPowerControlledSensorsData();
  //   Serial.println(fullMessage);
  //   delay(1000);

  //   if (attemptToSendLogs(fullMessage)) {
  //     Serial.println('[ok]');
  //     indicationSuccess();
  //   } else {
  //     Serial.println('[fail]');
  //     indicationFail();
  //   }

  //   delay(10000);
  // #else
  //   return;
  // #endif
}