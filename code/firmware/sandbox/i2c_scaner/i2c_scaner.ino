#include <Wire.h>
#include <Adafruit_INA219.h>

#define MODEM_POWER_PIN 12
Adafruit_INA219 ina219;


void initI2CSensors() {
  // if (!rtc.begin()) {
  //   Serial.println("E1: RTC DS3231 is not found!");
  // }

  // if (!bme.begin(0x76)) {
  //   Serial.println("E2: BME280 is not found!");
  // }
  
  // if (!ina219.begin()) {
  //   Serial.println("E3: Не удалось найти INA219");
  // }
}


void init() {
  //Wire.begin();
  //initAds();
  initI2CSensors();
}



void setup() {
  Wire.begin();
  Serial.begin(115200);
  
  pinMode(MODEM_POWER_PIN, OUTPUT);
  digitalWrite(MODEM_POWER_PIN, HIGH);
  delay(1000);

  init();
}


void loop() {
  byte error, address;
  int nDevices;

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Найдено I2C устройство по адресу 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
      nDevices++;
    } else if (error == 4) {
      Serial.print("Неизвестная ошибка по адресу 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0) {
    Serial.println("I2C устройств не найдено\n");
  } else {
    Serial.println("Сканирование завершено\n");
  }
  delay(5000);
}
