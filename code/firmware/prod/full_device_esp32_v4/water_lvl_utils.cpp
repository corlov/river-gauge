#include <Arduino.h>
#include "water_lvl_utils.h"
#include "water_lvl_settings.h"
#include "water_lvl_init.h"
#include "globals.h"
#include "water_lvl_types.h"
#include "errors.h"


/**
 * @brief Вспомогательная функция для извлечения N-ного значения из CSV-строки.
 * @param data Входная CSV-строка.
 * @param index Индекс нужного значения (начиная с 0).
 * @return Строка с найденным значением или пустая строка, если не найдено.
 */
String getNthValue(const String& data, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == ',' || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}



void modemOn() {
  digitalWrite(MODEM_POWER_PIN, HIGH);
  delay(2000);
}



void modemOff() {
  digitalWrite(MODEM_POWER_PIN, LOW);
  delay(2000);
}




/**
 * @brief Конвертирует строку формата "DD.MM.YYYY HH:MM:SS" в Unix Timestamp.
 * @param dateTimeStr Входная строка с датой и временем.
 * @return Unix Timestamp (uint64_t) или 0 в случае ошибки.
 */
uint64_t stringToTimestamp(const String& dateTimeStr) {
  struct tm tm;

  int items_scanned;
  items_scanned = sscanf(dateTimeStr.c_str(), "%d.%d.%d %d:%d:%d",
                            &tm.tm_mday, &tm.tm_mon, &tm.tm_year,
                            &tm.tm_hour, &tm.tm_min, &tm.tm_sec);

  if (items_scanned != 6) {
    return 0;
  }

  tm.tm_year -= 1900;
  tm.tm_mon -= 1;

  setenv("TZ", "UTC", 1);
  tzset();

  time_t timestamp = mktime(&tm);

  return (uint64_t)timestamp;
}



uint64_t convertToTimestamp(int year, int month, int day, int hour, int min, int sec) {
  // 1. Создаем структуру tm для хранения компонентов времени
  struct tm timeinfo;

  // 2. Заполняем структуру полученными данными
  timeinfo.tm_year = year - 1900; // tm_year считает годы от 1900
  timeinfo.tm_mon = month - 1;    // tm_mon считает месяцы от 0 (0=январь)
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour;
  timeinfo.tm_min = min;
  timeinfo.tm_sec = sec;
  timeinfo.tm_isdst = -1; // Автоматическое определение летнего времени (рекомендуется)

  // 3. Вызываем магическую функцию mktime(), которая делает всю работу
  // Она принимает локальное время и конвертирует его в time_t (что по сути и есть Unix Timestamp)
  time_t timestamp = mktime(&timeinfo);

  // 4. Возвращаем результат, преобразованный в uint64_t для совместимости с Protobuf
  return (uint64_t)timestamp;
}



/**
 * @brief Проверяет и устанавливает GPRS-соединение, если его нет.
 * @return true, если GPRS-соединение активно, иначе false.
 */
bool ensureGprsConnection() {
  SerialGSM.begin(9600, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
  
  if (!modem.restart()) {
    Serial.println("ОШИБКА: Не удалось перезагрузить модем!");
    blinkErrorCode(ERR_MODEM_RESTART);
    return false;
  }
  debugBlink(3, 50, 50);

  if (!modem.waitForNetwork(GSM_WAIT_TIMEOUT)) {
    blinkErrorCode(ERR_MODEM_NET_NOT_FOUND);
    Serial.println(" ОШИБКА: сеть не найдена.");
    return false;
  }
  debugBlink(3, 50, 50);

  if (!modem.gprsConnect(apn, gprs_user, gprs_pass)) {
    blinkErrorCode(ERR_MODEM_GPRS_CONNECT);
    Serial.println(" ОШИБКА: не удалось подключиться.");
    return false;
  }
  debugBlink(3, 50, 50);

  return true;
}
