#include "globals.h"
#include "water_lvl_init.h"
#include "water_lvl_settings.h"
#include "water_lvl_types.h"



void indicateWakeUp() {
  int i = 0;

  while (i++ < 5) {
    digitalWrite(LED_PIN, HIGH);  
    delay(800);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }

  Serial.println("Device is waked up\n");
}


void indicationInProgress() {
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
}



void indicationSuccess(int stage) {
  if (stage == 2) {
    int i = 0;
    while (i++ < 20) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  }
  else {
    int i = 0;
    while (i++ < 3) {
      digitalWrite(LED_PIN, HIGH);
      delay(1000);
      digitalWrite(LED_PIN, LOW);
      delay(1000);
    }
    delay(4000);
  }
}



void indicationFail() {
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
  pinMode(MODEM_POWER_PIN, OUTPUT);
  digitalWrite(MODEM_POWER_PIN, LOW);
  delay(1000);

  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW);

  // TODO: инициализировать неиспользуемые пины
  // for (int pin : unusedPins) {
  //   pinMode(pin, INPUT_PULLDOWN);
  // }
}



void initI2CSensors() {
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("E1: RTC DS3231 is not found!");
  }
  if (!bme.begin(0x76)) {
    Serial.println("E2: BME280 is not found!");
  }
  Serial.println("Sensors found [ok]");

  // раскоментировать 1 раз если надо синхронизовать датчик со времененм на ПК
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}



void init() {
  initPins();
  
  Serial.begin(9600);

  indicateWakeUp();

  initI2CSensors();
}

