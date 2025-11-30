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
  pinMode(LED_PIN, OUTPUT);

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
  if (!rtc.begin()) {
    Serial.println("E1: RTC DS3231 is not found!");
  }

  if (!bme.begin(0x76)) {
    Serial.println("E2: BME280 is not found!");
  }

  if (!ina219.begin()) {
    Serial.println("Не удалось найти INA219. Проверьте подключение!");
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


// void init() {
//   initPins();

//   initAds();
  
//   Serial.begin(9600);

//   indicateWakeUp();

//   initI2CSensors();
// }



void init() {
  initPins();
  Serial.begin(115200);

  indicateWakeUp();

  Wire.begin();
  Serial.println("I2C шина запущена.");

  initAds();
  initI2CSensors();
}