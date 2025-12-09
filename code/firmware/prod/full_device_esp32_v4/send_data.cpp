#include "send_data.h"
#include "globals.h"
#include "water_lvl_settings.h"
#include "storage.h"
#include "LittleFS.h"
#include "sensors.h"
#include "process_response.h"
#include <time.h> 
#include <PubSubClient.h>
#include "pb_encode.h"
#include "telemetry.pb.h"
#include "water_lvl_utils.h"
#include "water_lvl_init.h"
#include "broker.h"
#include "http.h"
#include "errors.h"




bool sendSms(const String& phoneNumber, const String& message) {
  if (phoneNumber.length() < 10) {
    blinkErrorCode(ERR_SMS);
    Serial.println("ОШИБКА SMS: Некорректный номер телефона.");
    return false;
  }
  if (message.length() == 0) {
    blinkErrorCode(ERR_SMS);
    Serial.println("ОШИБКА SMS: Пустое сообщение.");
    return false;
  }

  if (modem.sendSMS(phoneNumber, message)) {
    return true;
  } else {
    blinkErrorCode(ERR_SMS_SEND);
    Serial.println("ОШИБКА: Не удалось отправить SMS.");
    return false;
  }
}





/**
 * @brief Главная функция отправки. Выбирает метод (HTTP/MQTT) на основе настроек.
 * @param messageText Данные для HTTP (игнорируются для MQTT).
 * @param failCounter Счетчик неудач.
 * @return true в случае успеха.
 */
bool attemptToSend(String messageText, uint32_t failCounter) {
  
  bool wasSent = false;

  #ifdef HTTP_TRANSMIT_TYPE
    debugBlink(2, 300, 300);
    String payloadToSend = prepareLogPayload();
    wasSent = sendPayloadWithFallback(payloadToSend, messageText);
    debugBlink(2, 300, 300);
    if (wasSent) {
      LittleFS.remove(LOG_FILE_PATH);
      return true;
    }
  #endif 


  #ifdef MQTT_TRANSMIT_TYPE
    debugBlink(5, 300, 300);
    wasSent = attemptToSendMqtt(messageText);
    debugBlink(5, 300, 300);
    if (wasSent) {
      return true;
    }
  #endif 


  // пробуем смс отправить с самой важной информацией
  if (failCounter >= MAX_FAILS_SEND_COUNT) {
    String shortMsg = "Send error" + getPowerControlledSensorsData();
    sendSms(readStringSetting(SETTING_PHONE, DEFAULT_PHONE_NUMBER), shortMsg);
  }

  return false;
}




