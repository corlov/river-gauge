#include "water_lvl_types.h"
#include "water_lvl_settings.h"
#include "water_lvl_init.h"
#include "globals.h"
#include "storage.h"
#include "sensors.h"
#include "send_data.h"



int wl_measure_samples[SAMPLES_SIZE];

RTC_DS3231 rtc;
Adafruit_BME280 bme;

HardwareSerial SerialGSM(1);
TinyGsm modem(SerialGSM);
TinyGsmClient client(modem);

OneWire oneWire(WATER_TEMPERATURE_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

Adafruit_NeoPixel pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

Preferences preferences;



void setup() {
  init();
  
  PrevState prevState = getPrevState();

  String defaultSensorsData = getAlwaysOnSensorsData();

  if ((prevState.bootCount % DAILY_INTERVAL == 0) || (!prevState.success)) {
    indicationInProgress();

    // подаем питание на модем и цепи датчиков
    digitalWrite(MODEM_POWER_PIN, HIGH);
    delay(5000);

    String fullMessage = defaultSensorsData + getPowerControlledSensorsData();
    addCsvLine(fullMessage);

    if (attemptToSendLogs()) {
      setSuccess(true);
      indicationSuccess(2);
    } else {
      setSuccess(false);
      indicationFail();
    }
  }
  else {
    addCsvLine(defaultSensorsData);
    indicationSuccess(1);
  }

  powerOff();
}



void loop() {}