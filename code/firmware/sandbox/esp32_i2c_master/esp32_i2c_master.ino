// --- Обновленная функция loop() для ESP32 (Master) ---
void loop() {
  // --- Получаем данные от Pro Micro по I2C ---
  float waterLevel = 0.0;
  float batteryVoltage = 0.0;
  float waterTemperature = 0.0;

  // Запрашиваем у устройства с адресом 8 чуть больше данных (с запасом)
  Wire.requestFrom(PRO_MICRO_ADDRESS, 24);

  String response = "";
  while (Wire.available()) {
    char c = Wire.read();
    response += c;
  }

  // --- НОВАЯ ЛОГИКА ПАРСИНГА ДЛЯ ТРЕХ ЗНАЧЕНИЙ ---
  if (response.length() > 0) {
    // Находим первую запятую
    int firstComma = response.indexOf(',');
    // Находим вторую запятую, начиная поиск ПОСЛЕ первой
    int secondComma = response.indexOf(',', firstComma + 1);

    // Если обе запятые найдены
    if (firstComma > 0 && secondComma > firstComma) {
      String waterLevelStr = response.substring(0, firstComma);
      String batteryVoltageStr = response.substring(firstComma + 1, secondComma);
      String waterTempStr = response.substring(secondComma + 1);

      waterLevel = waterLevelStr.toFloat();
      batteryVoltage = batteryVoltageStr.toFloat();
      waterTemperature = waterTempStr.toFloat();
    }
  }

  // --- Используем полученные данные ---
  Serial.print("Получено по I2C -> Уровень: ");
  Serial.print(waterLevel);
  Serial.print(" м, Напряжение: ");
  Serial.print(batteryVoltage);
  Serial.print(" В, Температура: ");
  Serial.print(waterTemperature);
  Serial.println(" °C");

  // ... твой код отправки данных через SIM800 ...
  // Теперь ты можешь добавить waterTemperature в строку для отправки на сервер

  delay(10000);
}
