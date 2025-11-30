// --- ПРОШИВКА ДЛЯ ТЕСТА НА ARDUINO PRO MICRO ---

// --- КОНФИГУРАЦИЯ ДАТЧИКОВ И ПИНОВ ---

// 1. Датчик уровня воды (4-20мА)
#define WATER_LEVEL_PIN A0 // Используем аналоговый вход A0
#define SHUNT_RESISTOR 150.0
#define SENSOR_RANGE_METERS 5.0 // ЗАМЕНИ НА СВОЕ ЗНАЧЕНИЕ!

// 2. Датчик напряжения аккумулятора (Делитель)
#define BATTERY_VOLTAGE_PIN A1 // Используем аналоговый вход A1
#define R1 47000.0
#define R2 12000.0

// 3. КЛЮЧЕВЫЕ ИЗМЕНЕНИЯ ДЛЯ PRO MICRO
#define V_REF 5.0 // Опорное напряжение на Pro Micro - 5 Вольт
#define ADC_RESOLUTION 1023.0 // Разрешение АЦП на Pro Micro - 10 бит

// --- ФУНКЦИЯ SETUP ---
void setup() {

  pinMode(17, OUTPUT); // Используем пин 17 (RX LED)

  Serial.begin(9600); // Pro Micro работает через виртуальный COM-порт
  while (!Serial); // Ожидание открытия монитора порта
  delay(1000);
  Serial.println("--- Запущен тест на Arduino Pro Micro ---");
}

// --- ГЛАВНЫЙ ЦИКЛ ---
void loop() {
  float waterLevel = getWaterLevel();
  float batteryVoltage = getBatteryVoltage();

  Serial.print("Уровень воды: ");
  Serial.print(waterLevel, 2);
  Serial.print(" м  |  Напряжение АКБ: ");
  Serial.print(batteryVoltage, 2);
  Serial.println(" В");

  digitalWrite(17, HIGH);
  delay(1700); // Уменьшим задержку, чтобы быстрее увидеть результат
  digitalWrite(17, LOW);
  delay(1200);
}

// --- ФУНКЦИЯ ДЛЯ ДАТЧИКА УРОВНЯ ВОДЫ ---
float getWaterLevel() {
  int adcValue = analogRead(WATER_LEVEL_PIN);
  float voltage = (adcValue / ADC_RESOLUTION) * V_REF;
  float current_mA = (voltage / SHUNT_RESISTOR) * 1000.0;
  float useful_current = current_mA - 4.0;
  const float current_range = 16.0;
  float waterLevel = (useful_current / current_range) * SENSOR_RANGE_METERS;
  if (waterLevel < 0) {
    waterLevel = 0.0;
  }
  return waterLevel;
}

// --- ФУНКЦИЯ ДЛЯ ДАТЧИКА НАПРЯЖЕНИЯ АКБ ---
float getBatteryVoltage() {
  int adcValue = analogRead(BATTERY_VOLTAGE_PIN);
  float vout = (adcValue / ADC_RESOLUTION) * V_REF;
  float vin = vout * (R1 + R2) / R2;
  return vin;
}
