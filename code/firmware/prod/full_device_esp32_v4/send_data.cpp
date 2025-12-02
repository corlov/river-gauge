#include "send_data.h"
#include "globals.h"
#include "water_lvl_settings.h"
#include "storage.h"
#include "LittleFS.h"
#include "sensors.h"
#include "process_response.h"


bool sendSms(const String& phoneNumber, const String& message) {
  if (phoneNumber.length() < 10) {
    Serial.println("ОШИБКА SMS: Некорректный номер телефона.");
    return false;
  }
  if (message.length() == 0) {
    Serial.println("ОШИБКА SMS: Пустое сообщение.");
    return false;
  }

  Serial.print("Отправка SMS на номер: ");
  Serial.println(phoneNumber);
  Serial.print("Текст: ");
  Serial.println(message);

  // modem.sendSMS() возвращает true, если модуль подтвердил отправку.
  if (modem.sendSMS(phoneNumber, message)) {
    Serial.println("SMS успешно отправлена.");
    return true;
  } else {
    Serial.println("ОШИБКА: Не удалось отправить SMS.");
    return false;
  }
}



/**
 * @brief Проверяет и устанавливает GPRS-соединение, если его нет.
 * @return true, если GPRS-соединение активно, иначе false.
 */
bool ensureGprsConnection() {
  SerialGSM.begin(9600, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
  Serial.println("Инициализация модема...");
  if (!modem.restart()) {
    Serial.println("ОШИБКА: Не удалось перезагрузить модем!");
    return false;
  }
  Serial.println("Модем готов.");

  Serial.print("Ожидание сети...");
  if (!modem.waitForNetwork(30000L)) { // Увеличим таймаут ожидания сети до 30 сек
    Serial.println(" ОШИБКА: сеть не найдена.");
    return false;
  }
  Serial.println(" сеть найдена.");

  Serial.print("Подключение к GPRS...");
  if (!modem.gprsConnect(apn, gprs_user, gprs_pass)) {
    Serial.println(" ОШИБКА: не удалось подключиться.");
    return false;
  }

  Serial.println(" GPRS подключен.");
  return true;
}



/**
 * @brief Собирает сервисную информацию о состоянии модема и сети.
 * @return Строка с данными для добавления в основной пакет.
 */
String gatherChannelInfo() {
  int signalQuality = modem.getSignalQuality();
  String operatorName = modem.getOperator();
  int battPercent = modem.getBattPercent();
  int battVoltage = modem.getBattVoltage();
  String imei = modem.getIMEI();

  int year, month, day, hour, min, sec;
  float timezone;
  String formattedTime = "0000-00-00 00:00:00"; // Значение по умолчанию

  if (modem.getNetworkTime(&year, &month, &day, &hour, &min, &sec, &timezone)) {
    char timeBuffer[20];
    sprintf(timeBuffer, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, min, sec);
    formattedTime = String(timeBuffer);
  }

  return "," + String(signalQuality) + "," + operatorName + "," +
         String(battPercent) + "," + String(battVoltage) + "," + imei + "," +
         formattedTime;
}



/**
 * @brief Отправляет сформированное сообщение на указанный сервер по HTTP POST.
 * @param server Адрес сервера.
 * @param port Порт сервера.
 * @param message Тело HTTP-запроса (данные для отправки).
 * @return true, если сервер ответил "OK", иначе false.
 */
bool sendHttpRequest(const char* server, uint16_t port, const String& message) {
  if (!client.connect(server, port, 15000L)) {
    Serial.println("ОШИБКА: Не удалось подключиться к серверу.");
    return false;
  }

  Serial.println("Соединение с сервером установлено.");

  // Формируем и отправляем HTTP-запрос
  client.print(String("POST / HTTP/1.1\r\n"));
  client.print(String("Host: ") + server + "\r\n");
  client.print("Connection: close\r\n");
  client.print("Content-Type: text/csv\r\n");
  client.print(String("Content-Length: ") + message.length() + "\r\n");
  client.print("\r\n");
  client.print(message);

  Serial.println("Пакет данных отправлен. Ожидание ответа...");

  // Ожидание ответа от сервера
  unsigned long timeout = millis();
  String response_body = "";
  bool headers_ended = false;

  while (client.connected() && millis() - timeout < 15000L) {
    while (client.available()) {
      char c = client.read();
      if (headers_ended) {
        response_body += c;
      } else {
        // Простой способ найти конец заголовков
        static String header_line = "";
        header_line += c;
        if (header_line.endsWith("\r\n")) {
          if (header_line == "\r\n") {
            headers_ended = true;
          }
          header_line = "";
        }
      }
    }
  }
  client.stop();

  
  // if (response_body.indexOf("OK") != -1) {
  //   Serial.println("УСПЕХ! Сервер подтвердил получение данных.");
  //   return true;
  // } else {
  //   Serial.println("ОШИБКА: Не получен корректный ответ от сервера.");
  //   return false;
  // }

 
  return processServerResponse(response_body);
}



/**
 * @brief Главная функция для отправки данных с логикой резервирования.
 * Сначала пытается отправить на основной сервер. В случае неудачи - на резервный.
 * @param payloadLog Логирующая часть пакета.
 * @param payload Основная часть пакета данных.
 * @return true, если данные были успешно отправлены хотя бы на один из серверов.
 */
bool sendPayloadWithFallback(const String& payloadLog, const String& payload) {
  if (payload.length() == 0) {
    Serial.println("ОШИБКА: Пакет для отправки пуст, отмена.");
    return false;
  }

  if (!ensureGprsConnection()) {
    return false;
  }

  String channelInfo = gatherChannelInfo();
  String connection_dttm = "," + getDateTime();
  String fullMessage = payloadLog + payload + channelInfo + connection_dttm;

  if (sendHttpRequest(primary_server_address, primary_server_port, fullMessage)) {
    return true; 
  }

  if (sendHttpRequest(secondary_server_address, secondary_server_port, fullMessage)) {
    return true;
  }

  return false;
}



void modemOn() {
  digitalWrite(MODEM_POWER_PIN, HIGH);
  delay(2000);
}



void modemOff() {
  digitalWrite(MODEM_POWER_PIN, LOW);
  delay(2000);
}



bool attemptToSend(String messageText, uint32_t failCounter) {
  String payloadToSend = prepareLogPayload();

  bool wasSent = sendPayloadWithFallback(payloadToSend, messageText);

  if (wasSent) {
    LittleFS.remove(LOG_FILE_PATH);
    return true;
  }

  // если связь совсем плохая, пробуем смс отправить с самой важной информацией
  if (failCounter >= MAX_FAILS_SEND_COUNT) {
    String shortMsg = "Send error" + getPowerControlledSensorsData();
    sendSms(readStringSetting(SETTING_PHONE, DEFAULT_PHONE_NUMBER), shortMsg);
  }

  return false;
}

