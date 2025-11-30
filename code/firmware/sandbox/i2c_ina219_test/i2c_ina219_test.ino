#include <Wire.h>
#include <Adafruit_INA219.h>

#define MODEM_POWER_PIN 12
Adafruit_INA219 ina219;

void setup() {
  Serial.begin(115200);
  delay(1000); // Даем время Serial-порту стабилизироваться

  // --- ШАГ 1: ДЕРЖИМ СИЛОВУЮ ЦЕПЬ ВЫКЛЮЧЕННОЙ ---
  pinMode(MODEM_POWER_PIN, OUTPUT);
  digitalWrite(MODEM_POWER_PIN, LOW);
  Serial.println("Силовая цепь 12В пока выключена.");

  // --- ШАГ 2: В ПОЛНОЙ ТИШИНЕ ИНИЦИАЛИЗИРУЕМ I2C ---
  Wire.begin();
  Serial.println("I2C шина запущена.");

  // Пытаемся начать "сложный разговор" с INA219
  if (!ina219.begin()) {
    Serial.println("КРИТИЧЕСКАЯ ОШИБКА: INA219 не найден ДАЖЕ в тишине. Проблема в железе.");
    while(1); // Останавливаемся, если что-то не так
  } else {
    Serial.println("УСПЕХ! INA219 успешно инициализирован в тишине.");
  }


  delay(3000);
  getVoltData();
  delay(3000);

  // --- ШАГ 3: ТЕПЕРЬ, КОГДА ВСЕ ГОТОВО, ВКЛЮЧАЕМ 12В ---
  Serial.println("\nВключаю силовую цепь 12В...");
  digitalWrite(MODEM_POWER_PIN, HIGH);
  delay(1000); // Даем время на стабилизацию
  Serial.println("Силовая цепь включена.");

  // --- ШАГ 4: ПРОВЕРЯЕМ, ВСЕ ЛИ ЖИВЫ ПОСЛЕ УДАРА ---
  Serial.println("\nЗапускаю сканер для проверки состояния ПОСЛЕ включения 12В...");
}


String getVoltData() {
  float bus_voltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();

  Serial.print("U: "); Serial.print(bus_voltage); Serial.print(" В, ");
  Serial.print("I: "); Serial.print(current_mA); Serial.print(" мА, ");
  Serial.print("P: "); Serial.print(power_mW); Serial.println(" мВт");

  String data = ",";
  data += String(bus_voltage, 2) + ",";
  data += String(current_mA, 2) + ",";
  data += String(power_mW, 2);

  return "";
}

void loop() {
  // Здесь остался твой рабочий сканер, который покажет нам,
  // что все датчики по-прежнему на месте.
  byte error, address;
  int nDevices;

  Serial.println("\n--- Сканирование ---");
  nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("Найдено I2C устройство по адресу 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }
  if (nDevices == 0) {
    Serial.println("I2C устройств не найдено\n");
  } else {
    Serial.println("Сканирование завершено\n");
  }

  getVoltData();
  delay(5000);
}
