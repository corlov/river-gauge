//#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    // Ждем, пока откроется Serial-монитор
  }

  Serial.println("--- Тест модуля INA219 на Arduino Nano ---");

  // Инициализируем модуль
  if (!ina219.begin()) {
    Serial.println("Не удалось найти INA219. Проверьте подключение!");
    while (1) { delay(10); } // Останавливаемся, если датчик не найден
  }
  Serial.println("Модуль INA219 найден!");
}

void loop() {
  float bus_voltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();

  Serial.print("U: "); Serial.print(bus_voltage); Serial.print(" В, ");
  Serial.print("I: "); Serial.print(current_mA); Serial.print(" мА, ");
  Serial.print("P: "); Serial.print(power_mW); Serial.println(" мВт");

  delay(5000);
}
