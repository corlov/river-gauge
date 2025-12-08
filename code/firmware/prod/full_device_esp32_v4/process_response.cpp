#include "process_response.h"
#import "globals.h"
#import "water_lvl_settings.h"
#import "storage.h"
#include "water_lvl_init.h"



/**
 * @brief Парсит строку времени из формата YYYY-MM-DDTHH:MM:SS и обновляет RTC
 * @param timeString Строка времени от сервера.
 */
bool updateRtcFromServerTime(const char* timeString) {
  if (timeString == nullptr) {
    Serial.println("Время от сервера не получено.");
    indicationErrore(ERR_CODE_UPDATE_RTC);
    return false;
  }

  int year, month, day, hour, minute, second;
  if (sscanf(timeString, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    delay(1000);
  } else {
    indicationErrore(ERR_CODE_UPDATE_RTC);
    Serial.println("ОШИБКА: не удалось распарсить время от сервера.");
    return false;
  }

  return true;
}



/**
 * @brief Обрабатывает JSON-ответ от сервера, сохраняет настройки и обновляет время.
 * @param responseBody Тело HTTP-ответа от сервера.
 * @return true, если ответ корректен (status == "OK"), иначе false.
 */
bool processServerResponse(const String& responseBody) {
  // Выделяем память для JSON-документа. 384 байт должно хватить с запасом.
  StaticJsonDocument<384> doc;

  DeserializationError error = deserializeJson(doc, responseBody);

  if (error) {
    indicationErrore(ERR_CODE_RESPONSE);
    Serial.print("ОШИБКА: не удалось распарсить JSON: ");
    Serial.println(error.c_str());

    // Если не смогли распарсить, но ответ содержит "OK", считаем это успехом (для обратной совместимости)
    return responseBody.indexOf("OK") != -1;
  }

  const char* status = doc["status"];
  if (status == nullptr || strcmp(status, "OK") != 0) {
    indicationErrore(ERR_CODE_RESPONSE);
    Serial.println("ОШИБКА: в ответе сервера нет статуса 'OK'.");
    return false;
  }

  JsonObject config = doc["config"];

  // Используем оператор `|` - если ключа нет, значение не изменится.
  writeIntSetting(SETTING_ACTIVATION_FREQ, config["MODEM_ACTIVATION_FREQENCY"] | readIntSetting(SETTING_ACTIVATION_FREQ, DEFAULT_MODEM_ACTIVATION_FREQENCY));
  writeFloatSetting(SETTING_CURRENT_MIN_MA, config["CURRENT_MIN_MA"] | readFloatSetting(SETTING_CURRENT_MIN_MA, DEFAULT_CURRENT_MIN_MA));
  writeFloatSetting(SETTING_RESISTOR_OHMS, config["RESISTOR_OHMS"] | readFloatSetting(SETTING_RESISTOR_OHMS, DEFAULT_RESISTOR_OHMS));
  writeStringSetting(SETTING_PHONE, config["PHONE_NUMBER"] | readStringSetting(SETTING_PHONE, DEFAULT_PHONE_NUMBER));

  return updateRtcFromServerTime(config["current_server_time"]);
}








/**
 * @brief Обрабатывает JSON с настройками, полученный по MQTT.
 * @param responseBody JSON-строка с настройками.
 * @return true, если JSON был успешно разобран, иначе false.
 */
bool processServerResponseMqtt(const String& responseBody) {
  // Выделяем память для JSON-документа.
  StaticJsonDocument<384> doc;

  DeserializationError error = deserializeJson(doc, responseBody);

  if (error) {
    indicationErrore(ERR_CODE_RESPONSE);
    Serial.print("ОШИБКА: не удалось распарсить JSON: ");
    Serial.println(error.c_str());
    return false; // Возвращаем false, если JSON некорректен
  }


  if (doc.containsKey("MODEM_ACTIVATION_FREQENCY")) {
    writeIntSetting(SETTING_ACTIVATION_FREQ, doc["MODEM_ACTIVATION_FREQENCY"]);
    Serial.println(" - Обновлен MODEM_ACTIVATION_FREQENCY");
  }

  if (doc.containsKey("CURRENT_MIN_MA")) {
    writeFloatSetting(SETTING_CURRENT_MIN_MA, doc["CURRENT_MIN_MA"]);
    Serial.println(" - Обновлен CURRENT_MIN_MA");
  }

  if (doc.containsKey("RESISTOR_OHMS")) {
    writeFloatSetting(SETTING_RESISTOR_OHMS, doc["RESISTOR_OHMS"]);
    Serial.println(" - Обновлен RESISTOR_OHMS");
  }

  if (doc.containsKey("PHONE_NUMBER")) {
    writeStringSetting(SETTING_PHONE, doc["PHONE_NUMBER"].as<String>());
    Serial.println(" - Обновлен PHONE_NUMBER");
  }

  // --- Обработка времени (если оно будет добавлено в JSON) ---
  // Этот блок теперь не влияет на возвращаемое значение функции.
  if (doc.containsKey("current_server_time")) {
    if (updateRtcFromServerTime(doc["current_server_time"])) {
      Serial.println("Время RTC успешно синхронизировано.");
    } else {
      Serial.println("ОШИБКА: не удалось синхронизировать время RTC.");
    }
  }

  Serial.println("Обработка настроек завершена.");
  return true; // Возвращаем true, так как парсинг прошел успешно
}




/**
 * @brief Callback-функция, которая будет вызвана ТОЛЬКО при получении настроек.
 */
void settingsCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("--- Получен ответ с настройками! ---");
  String responseBody;
  responseBody.reserve(length);
  for (int i = 0; i < length; i++) {
    responseBody += (char)payload[i];
  }

  if (processServerResponseMqtt(responseBody)) {
    Serial.println("Настройки успешно обработаны.");
  } else {
    Serial.println("ОШИБКА обработки настроек.");
  }

  // Устанавливаем флаг, что мы получили ответ
  settingsReceived = true;
}





