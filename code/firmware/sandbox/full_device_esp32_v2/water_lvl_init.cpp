#include "globals.h"
#include "water_lvl_init.h"
#include "water_lvl_settings.h"
#include "water_lvl_types.h"


void indicateWakeUp() {
  int i = 0;
  while (i < 3) {
    pixels.setPixelColor(0, CL_WHITE);
    pixels.show();  
    delay(500);

    pixels.setPixelColor(0, CL_OFF);
    pixels.show();  
    delay(500);
  }  
  Serial.println("Device is waked up\n");
}


void indicationInProgress() {
  pixels.setPixelColor(0, CL_BLUE);
  pixels.show();
}



void indicationSuccess(int stage) {
  if (stage == 2) {
    int i = 0;
    while (i < 10) {
      pixels.setPixelColor(0, CL_PURPLE);
      pixels.show();
      delay(200);

      pixels.setPixelColor(0, CL_GREEN);
      pixels.show();  
      delay(200);
    }
    pixels.setPixelColor(0, CL_OFF);
  }
  else {
    pixels.setPixelColor(0, CL_GREEN);
    pixels.show();  
    delay(4000);
  }
}



void indicationFail() {
  pixels.setPixelColor(0, CL_RED);
  pixels.show();
  delay(4000);
}




void initPins() {
  pixels.begin();
  pixels.setBrightness(50);

  pinMode(MODEM_POWER_PIN, OUTPUT);
  digitalWrite(MODEM_POWER_PIN, LOW);

  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW);

  for (int pin : unusedPins) {
    pinMode(pin, INPUT_PULLDOWN);
  }
}


void initI2CSensors() {
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println("E1: RTC DS3231 is not found!");
  }
  if (!bme.begin(0x76)) {
    Serial.println("E2: BME280 is not found!");
  }

  // раскоментировать 1 раз если надо синхронизовать датчик со времененм на ПК
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}



void init() {
  initPins();
  
  Serial.begin(9600);

  indicateWakeUp();

  initI2CSensors();
}

