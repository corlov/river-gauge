#include "process_response.h"
#include "globals.h"
#include "water_lvl_settings.h"
#include "storage.h"
#include "water_lvl_init.h"
#include "errors.h"


/**
 * @brief Парсит строку времени из формата YYYY-MM-DDTHH:MM:SS и обновляет RTC
 * @param timeString Строка времени от сервера.
 */
bool updateRtcFromServerTime(const char* timeString) {
  if (timeString == nullptr) {
    Serial.println("Время от сервера не получено.");
    blinkErrorCode(ERR_CODE_UPDATE_RTC);
    return false;
  }

  int year, month, day, hour, minute, second;
  if (sscanf(timeString, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {
    rtc.adjust(DateTime(year, month, day, hour, minute, second));
    delay(1000);
  } else {
    blinkErrorCode(ERR_CODE_UPDATE_RTC_PARSE);
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
  StaticJsonDocument<JSON_BUFF_SIZE> doc;

  DeserializationError error = deserializeJson(doc, responseBody);

  if (error) {
    blinkErrorCode(ERR_CODE_RESPONSE);
    Serial.print("ОШИБКА: не удалось распарсить JSON: ");
    Serial.println(error.c_str());
    return false;
  }

  const char* status = doc["status"];
  if (status == nullptr || strcmp(status, "OK") != 0) {
    blinkErrorCode(ERR_CODE_RESPONSE);
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
  StaticJsonDocument<JSON_BUFF_SIZE> doc;

  DeserializationError error = deserializeJson(doc, responseBody);

  if (error) {
    blinkErrorCode(ERR_CODE_RESPONSE);
    Serial.print("ОШИБКА: не удалось распарсить JSON: ");
    Serial.println(error.c_str());
    return false;
  }

  if (doc.containsKey("MODEM_ACTIVATION_FREQENCY")) {
    writeIntSetting(SETTING_ACTIVATION_FREQ, doc["MODEM_ACTIVATION_FREQENCY"]);
  }

  if (doc.containsKey("CURRENT_MIN_MA")) {
    writeFloatSetting(SETTING_CURRENT_MIN_MA, doc["CURRENT_MIN_MA"]);
  }

  if (doc.containsKey("RESISTOR_OHMS")) {
    writeFloatSetting(SETTING_RESISTOR_OHMS, doc["RESISTOR_OHMS"]);
  }

  if (doc.containsKey("PHONE_NUMBER")) {
    writeStringSetting(SETTING_PHONE, doc["PHONE_NUMBER"].as<String>());
  }


  if (doc.containsKey("current_server_time")) {
    if (!updateRtcFromServerTime(doc["current_server_time"])) {
      Serial.println("ОШИБКА: не удалось синхронизировать время RTC.");
      return false;
    }
  }

  return true;
}




/**
 * @brief Callback-функция, которая будет вызвана ТОЛЬКО при получении настроек.
 */
void settingsCallback(char* topic, byte* payload, unsigned int length) {
  String responseBody;
  responseBody.reserve(length);
  for (int i = 0; i < length; i++) {
    responseBody += (char)payload[i];
  }

  if (!processServerResponseMqtt(responseBody)) {
    blinkErrorCode(ERR_MQTT_APPLY_CONFIG);
    Serial.println("ОШИБКА обработки настроек.");
  }

  settingsReceived = true;
}





