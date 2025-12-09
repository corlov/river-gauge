#include "sensors.h"
#include "water_lvl_settings.h"
#include "water_lvl_types.h"
#include "water_lvl_init.h"
#include "storage.h"
#include "errors.h"


String getBmeData() {
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  String data = "";
  data += String(temperature, 1) + ",";
  data += String(humidity, 1) + ",";
  data += String(pressure, 1);

  return data;
}



String getPowerData() {
  float bus_voltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();

  #ifdef DEBUG_MODE
    Serial.printf("U: %.2f В, I: %.2f мА, P: %.2f мВт\n", bus_voltage, current_mA, power_mW);
  #endif

  String data = ",";
  data += String(bus_voltage, 2) + ",";
  data += String(current_mA, 2) + ",";
  data += String(power_mW, 2);

  return data;
}


void powerOff() {
    delay(1000);
    digitalWrite(DONE_PIN, HIGH);
    delay(200);
    digitalWrite(DONE_PIN, LOW);
}



String getDateTime() {
  DateTime now = rtc.now();
  char timestamp[20];
  sprintf(timestamp, "%02d.%02d.%04d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());

  return String(timestamp);
}



String getAlwaysOnSensorsData() {    
    float temperatureFromRtcSensor = rtc.getTemperature();
    float waterTemperature = getWaterTemperature();

    String data = getDateTime() + "," + getBmeData() + "," + String(temperatureFromRtcSensor,1) + "," + String(waterTemperature, 1) + "," + String(DEFAULT_DEVICE_ID);
    data += "," + String(GPS_LON, 7) + "," + String(GPS_LAT, 7) + "," + FIRMWARE_VERSION + "," + INSTALL_DATE;

    return data;
}



String getPowerControlledSensorsData() {
    String data = "";

    float waterLevel = getActualWaterLevel();

    data += "," + String(waterLevel, 3) + getPowerData();

    return data;
}




/**
 * @brief Считывает показания с датчика, фильтрует их и возвращает уровень воды.
 *
 * Функция выполняет NUM_READINGS (13) измерений, сортирует их и возвращает
 * медианное (центральное) значение для устранения случайных выбросов.
 *
 * @return float - Уровень воды в метрах. Возвращает -1.0 в случае ошибки (обрыв цепи).
 */
float getActualWaterLevelMeasure() {
  float readings[NUM_READINGS]; 

  // 1. Собираем 13 показаний
  for (int i = 0; i < NUM_READINGS; i++) {
    int16_t adc_raw = ads.readADC_SingleEnded(0);
    float volts = ads.computeVolts(adc_raw);
    float current_mA = (volts / readFloatSetting(SETTING_RESISTOR_OHMS, DEFAULT_RESISTOR_OHMS)) * 1000.0;

    float levelMeters;
    if (current_mA < (readFloatSetting(SETTING_CURRENT_MIN_MA, DEFAULT_CURRENT_MIN_MA) - 0.5)) { // Проверка на обрыв цепи
      levelMeters = -1.0; // Ошибка или обрыв
      
    } else {
      // Используем функцию map для перевода диапазона тока в диапазон уровня
      long current_micro_amps = current_mA * 1000;
      long min_micro_amps = readFloatSetting(SETTING_CURRENT_MIN_MA, DEFAULT_CURRENT_MIN_MA) * 1000;
      long max_micro_amps = CURRENT_MAX_MA * 1000;
      long level_cm = map(current_micro_amps, min_micro_amps, max_micro_amps, SENSOR_LEVEL_MIN_METERS * 100, SENSOR_LEVEL_MAX_METERS * 100);
      levelMeters = level_cm / 100.0;
    }
    readings[i] = levelMeters;
    delay(20);
  }

  // 2. Сортируем массив показаний (простой "пузырьковый" метод)
  for (int i = 0; i < NUM_READINGS - 1; i++) {
    for (int j = 0; j < NUM_READINGS - i - 1; j++) {
      if (readings[j] > readings[j + 1]) {
        float temp = readings[j];
        readings[j] = readings[j + 1];
        readings[j + 1] = temp;
      }
    }
  }

  // 3. Возвращаем центральный элемент. Для 13 элементов это 7-й (индекс 6)
  return readings[NUM_READINGS / 2];
}



float getActualWaterLevel() {
  float waterLevel = getActualWaterLevelMeasure();

  if (waterLevel < 0) {
    Serial.println("E5: water level sensor error");
    blinkErrorCode(ERR_CODE_WATER_LEVEL_ERROR);
    return -1024;
  } 
  return waterLevel;
}



float getWaterTemperature() {
  sensors.begin();
  delay(1000);
  sensors.requestTemperatures();

  float tempC = sensors.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("E4: get water temperature error");
    blinkErrorCode(ERR_CODE_WATER_TEMP);
    return 65535;
  }

  return tempC;
}
