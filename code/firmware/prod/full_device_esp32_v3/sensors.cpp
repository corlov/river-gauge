#import "sensors.h"
#import "water_lvl_settings.h"


String getBmeData() {
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  String data = "";
  data += String(temperature, 1) + ",";
  data += String(humidity, 1) + ",";
  data += String(pressure, 1);

  Serial.println("bme t:  " + String(temperature,1));
  Serial.println("bme hum : " + String(humidity,1));
  Serial.println("bme Pres: " + String(pressure,1));

  return data;
}


void powerOff() {
    delay(1000);
    digitalWrite(DONE_PIN, HIGH);
    delay(200);
    digitalWrite(DONE_PIN, LOW);
}



String getAlwaysOnSensorsData() {
    DateTime now = rtc.now();
    char timestamp[20];
    sprintf(timestamp, "%02d.%02d.%04d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());

    float temperatureFromRtcSensor = rtc.getTemperature();

    float waterTemperature = getWaterTemperature();

    String data = String(timestamp) + "," + getBmeData() + "," + String(temperatureFromRtcSensor,1) + "," + String(waterTemperature, 1) + "," + String(DEVICE_ID);
    data += "," + String(GPS_LON, 7) + "," + String(GPS_LAT, 7) + "," + VERSION + "," + INSTALL_DATE;

    
    

    Serial.println("RTC t:  " + String(temperatureFromRtcSensor,1));
    Serial.println("Water t : " + String(waterTemperature,1));
    Serial.println("=========================");

    //Serial.println(data);

    // FIXME: TODO: дополнить в конце посылку этими данными
    // сколько секунд искалась связь
    // качество сигнала
    // попытка с которой это сообщение передано
  
    return data;
}


String getPowerControlledSensorsData() {
    String data = "";
    float batVoltage = readBatteryVoltage();
    float waterLevel = getActualWaterLevel();

    //String msg = "batVoltage: " + String(batVoltage, 2) + ", waterLevel: " + String(waterLevel, 2);
    Serial.println("batVoltage: " + String(batVoltage, 2) + ", waterLevel: " + String(waterLevel, 2));

    data += "," + String(batVoltage, 1);
    data += "," + String(waterLevel, 1);

    return data;
}


float getActualWaterLevel() {
  for (int i = 0; i < SAMPLES_SIZE; i++) {
    wl_measure_samples[i] = analogRead(WATER_LEVEL_SENSOR_PIN);
    delay(20);
  }

  for (int i = 0; i < SAMPLES_SIZE - 1; i++) {
    for (int j = 0; j < SAMPLES_SIZE - i - 1; j++) {
      if (wl_measure_samples[j] > wl_measure_samples[j + 1]) {
        int temp = wl_measure_samples[j];
        wl_measure_samples[j] = wl_measure_samples[j + 1];
        wl_measure_samples[j + 1] = temp;
      }
    }
  }

  // FIXME: я бы сырое значение тоже передавал, хотя его можно получить из данных
  float medianRawValue = wl_measure_samples[8];

  Serial.print("Сырое значение АЦП уровня воды: ");
  Serial.println(medianRawValue);

  // --- Шаг 4: Преобразуем медианное значение в напряжение ---
  float voltage = (medianRawValue / 4095.0) * V_REF; // для ПроМикро тут 1023 т.к.10битный у них АЦП

  // --- Шаг 5: Преобразуем напряжение в ток (в миллиамперах) ---
  float current_mA = (voltage / WATER_LEVEL_SHUNT_RESISTOR_VALUE) * 1000.0;

  // --- Шаг 6: Преобразуем ток в уровень воды (в метрах) ---
  // описывает физический стандарт 4-20 мА датчика, а не способ измерения
  float useful_current = current_mA - 4.0; // При 0% измеряемой величины (минимальный уровень воды) он физически выдает ток 4 мА
  const float current_range = 16.0; // (20.0 - 4.0)
  float waterLevel = (useful_current / current_range) * WATER_LEVEL_SENSOR_RANGE_METERS;

  if (waterLevel < 0) {
    waterLevel = 0.0;
  }

  return waterLevel;
}



float getWaterTemperature() {
  sensors.begin();
  delay(1000);
  sensors.requestTemperatures(); // Эта команда не блокирует выполнение

  // Получаем температуру с первого найденного на шине датчика (индекс 0)
  // и выводим ее.
  float tempC = sensors.getTempCByIndex(0);

  // Проверяем, не вернул ли датчик ошибку
  // (значение -127 означает, что датчик не найден или неисправен)
  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("E4: get water temperature error");
    delay(1000); // Ждем секунду перед новой попыткой
    return 65535;
  }

  return tempC;
}



/**
 * @brief Включает делитель, измеряет напряжение и выключает делитель.
 * @return Напряжение аккумулятора в Вольтах.
 */
float readBatteryVoltage() {
  // 3. Считываем значение с АЦП (0-1023)
  int adcValue = analogRead(BATTERY_VOLTAGE_PIN);

  // 5. Конвертируем значение АЦП обратно в напряжение
  // Сначала находим напряжение на пине A0
  //float dividerVoltage = adcValue * (5.0 / 1023.0);
  float dividerVoltage = adcValue * (V_REF / 4095.0);

  // Затем, зная напряжение на делителе, вычисляем исходное напряжение аккумулятора
  // Формула: V_in = V_out * (R1 + R2) / R2
  float batteryVoltage = dividerVoltage * (VOLTAGE_DIVIDER_UP_RESISTOR + VOLTAGE_DIVIDER_DOWN_RESISTOR) / VOLTAGE_DIVIDER_DOWN_RESISTOR;

  return batteryVoltage;
}


void printProMicroData() {
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
}

