#include "http.h"
#include "globals.h"
#include "water_lvl_settings.h"
#include "water_lvl_init.h"
#include "process_response.h"
#include "water_lvl_utils.h"
#include "sensors.h"
#include "water_lvl_types.h"
#include "errors.h"



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
    char timeBuffer[32];
    snprintf(timeBuffer, sizeof(timeBuffer), "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, min, sec);
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
    blinkErrorCode(ERR_HTTP_CONNECT);
    return false;
  }


  // Формируем и отправляем HTTP-запрос
  client.print(String("POST / HTTP/1.1\r\n"));
  client.print(String("Host: ") + server + "\r\n");
  client.print("Connection: close\r\n");
  client.print("Content-Type: text/csv\r\n");
  client.print(String("Content-Length: ") + message.length() + "\r\n");
  client.print("\r\n");
  client.print(message);


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
    blinkErrorCode(ERR_HTTP_EMPTY_PKG);
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


