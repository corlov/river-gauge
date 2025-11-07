// Пины для отключаемого делителя
#define VOLTAGE_PIN A1      // Пин для считывания напряжения
#define DIVIDER_CTRL_PIN 7  // Пин для управления MOSFET'ом

// Номиналы резисторов (используйте .0, чтобы заставить компилятор считать в float)
const float R1 = 17500.0;
const float R2 = 10000.0;

void setup() {
  Serial.begin(9600);

  // Устанавливаем пин управления как выход
  pinMode(DIVIDER_CTRL_PIN, OUTPUT);

  // Убедимся, что делитель выключен при старте
  digitalWrite(DIVIDER_CTRL_PIN, LOW);
}

void loop() {
  // Вызываем функцию для замера напряжения
  float batteryVoltage = readBatteryVoltage();

  // Выводим результат в монитор порта
  Serial.print("Battery Voltage: ");
  Serial.print(batteryVoltage);
  Serial.println(" V");

  // Ждем 10 секунд до следующего замера
  delay(2000);
}

/**
 * @brief Включает делитель, измеряет напряжение и выключает делитель.
 * @return Напряжение аккумулятора в Вольтах.
 */
float readBatteryVoltage() {
  // 1. Включаем делитель
  digitalWrite(DIVIDER_CTRL_PIN, HIGH);

  // 2. Даем напряжению стабилизироваться (очень короткая пауза)
  delay(2);

  // 3. Считываем значение с АЦП (0-1023)
  int adcValue = analogRead(VOLTAGE_PIN);

  delay(500);
  // 4. Сразу же выключаем делитель для экономии энергии
  digitalWrite(DIVIDER_CTRL_PIN, LOW);

  // 5. Конвертируем значение АЦП обратно в напряжение
  // Сначала находим напряжение на пине A0
  float dividerVoltage = adcValue * (5.0 / 1023.0);

  // Затем, зная напряжение на делителе, вычисляем исходное напряжение аккумулятора
  // Формула: V_in = V_out * (R1 + R2) / R2
  float batteryVoltage = dividerVoltage * (R1 + R2) / R2;

  return batteryVoltage;
}
