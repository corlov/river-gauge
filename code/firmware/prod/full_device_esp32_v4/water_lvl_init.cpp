#include "globals.h"
#include "water_lvl_init.h"
#include "water_lvl_settings.h"
#include "water_lvl_types.h"



void indicateWakeUp() {

  digitalWrite(LED_PIN, HIGH);  
  delay(1000);
  digitalWrite(LED_PIN, LOW);

  // int i = 0;

  // while (i++ < 5) {
  //   digitalWrite(LED_PIN, HIGH);  
  //   delay(800);
  //   digitalWrite(LED_PIN, LOW);
  //   delay(200);
  // }

  // Serial.println("Device is waked up\n");
}



void indicationSuccess() {
  int i = 0;
  while (i++ < 20) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }  
}


void indicationSuccessWithoutSend() {
  int i = 0;
  while (i++ < 10) {
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }  
}


void indicationErrore(int erroreCode) {
  int k = 0;
  while (k++ < 3) {
    int i = 0;
    while (i++ < erroreCode) {
      digitalWrite(ERRORE_LED_PIN, HIGH);
      delay(500);
      digitalWrite(ERRORE_LED_PIN, LOW);
      delay(500);
    }
    delay(2000);
  }
}


void indicationFail() {
  indicationErrore(ERR_CODE_SEND_ERROR);
  int i = 0;
  while (i++ < 3) {
    digitalWrite(LED_PIN, HIGH);
    delay(3000);
    digitalWrite(LED_PIN, LOW);
    delay(500);
  }
  delay(4000);
}




void initPins() {
  pinMode(LED_PIN, OUTPUT);

  pinMode(MODEM_POWER_PIN, OUTPUT);
  digitalWrite(MODEM_POWER_PIN, LOW);  

  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW);

  pinMode(ERRORE_LED_PIN, OUTPUT);
  digitalWrite(ERRORE_LED_PIN, LOW);

  for (int pin : unusedPins) {
    pinMode(pin, INPUT_PULLDOWN);
  }
}



void initI2CSensors() {
  if (!rtc.begin()) {
    Serial.println("E1: RTC DS3231 is not found!");
    indicationErrore(ERR_CODE_RTC);
  }

  if (!bme.begin(0x76)) {
    Serial.println("E2: BME280 is not found!");
    indicationErrore(ERR_CODE_BME);
  }

  if (!ina219.begin()) {
    Serial.println("E3: INA219 is not found!");
    indicationErrore(ERR_CODE_INA219);
  }

  // раскоментировать 1 раз если надо синхронизовать датчик со времененм на ПК
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}


void initAds() {
  ads.begin();
  // Усиление GAIN_TWO подходит для диапазона +/- 2.048V.
  // Наш сигнал 0.4-2.0V идеально в него вписывается.
  ads.setGain(GAIN_TWO);
}



void init() {
  initPins();
  Serial.begin(115200);

  indicateWakeUp();

  Wire.begin();
  initAds();
  initI2CSensors();
}