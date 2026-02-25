#include "water_lvl_types.h"
#include "water_lvl_settings.h"
#include "water_lvl_init.h"
#include "globals.h"
#include "storage.h"
#include "sensors.h"
#include "send_data.h"
#include <esp_task_wdt.h>

// WDT_TIMEOUT секунд максимально евермя работы утсройства, иначе принудительно оно перезагрузится
#define WDT_TIMEOUT 120

#define CONFIG_FREERTOS_NUMBER_OF_CORES 1 

// Fixes complier error “invalid conversion from ‘int’ to ‘const esp_task_wdt_config_t*'”:
esp_task_wdt_config_t twdt_config = 
    {
        .timeout_ms = WDT_TIMEOUT * 1000,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,    // Bitmask of cores
        .trigger_panic = true,
    };



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

bool settingsReceived = false;

PubSubClient mqttClient(client);

//#define DEBUG_MODE 1





void setup() {
  esp_task_wdt_deinit();
  esp_err_t err = esp_task_wdt_init(&twdt_config);
  if (err != ESP_OK) {
    
  }
  esp_task_wdt_add(NULL);
  esp_task_wdt_reset();

  

  
  // #ifdef DEBUG_MODE
    // init();
    // modemOn();
    // return;
  // #endif

  init();
  delay(5000);


  // если предыдущий запуск ничем не закончился выключаем девайс
  if (! get_prev_start_ok()) {
    set_prev_start_ok(true);
    powerOff();
    return;
  }
  set_prev_start_ok(false);
  

  PrevState prevState = loadAndIncrementBootState();
  

  bool needToSend = ((prevState.bootCount - 1) % readIntSetting(SETTING_ACTIVATION_FREQ, DEFAULT_MODEM_ACTIVATION_FREQENCY) == 0) || (!prevState.success);
  //if (needToSend) {
  if (1) {
    setSuccess(false);

    modemOn();
    delay(2000);

    String message = String(prevState.bootCount) + "," + getAlwaysOnSensorsData() + getPowerControlledSensorsData();

    //addCsvLine(message);

    if (attemptToSend(message, prevState.failCounter)) {
      setSuccess(true);
      indicationSuccess();
    } else {
      indicationFail();
    }

    modemOff();
  }
  else {
    addCsvLine(String(prevState.bootCount) + "," + getAlwaysOnSensorsData() + getPowerControlledSensorsDataDump());
    indicationSuccessWithoutSend();
  }

  set_prev_start_ok(true);
  powerOff();

  esp_task_wdt_reset();
}



void loop() {
  // #ifdef DEBUG_MODE


    // String defaultSensorsData = getAlwaysOnSensorsData();
    // String fullMessage = defaultSensorsData + getPowerControlledSensorsData();
    // Serial.println("===================");
    // Serial.println(fullMessage);
    // Serial.println("");
    // Serial.println("");
    // delay(5000);
    
    // modemOff();
    // delay(3000);
    // powerOff();




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