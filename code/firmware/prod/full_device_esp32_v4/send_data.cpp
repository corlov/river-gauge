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


PubSubClient mqttClient(client);


bool sendSms(const String& phoneNumber, const String& message) {
  if (phoneNumber.length() < 10) {
    Serial.println("ОШИБКА SMS: Некорректный номер телефона.");
    return false;
  }
  if (message.length() == 0) {
    Serial.println("ОШИБКА SMS: Пустое сообщение.");
    return false;
  }

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
  
  if (!modem.restart()) {
    Serial.println("ОШИБКА: Не удалось перезагрузить модем!");
    debugBlink(2, 500, 500);
    return false;
  }

  if (!modem.waitForNetwork(GSM_WAIT_TIMEOUT)) { // Увеличим таймаут ожидания сети до 70 сек
    debugBlink(3, 500, 500);
    Serial.println(" ОШИБКА: сеть не найдена.");
    return false;
  }

  if (!modem.gprsConnect(apn, gprs_user, gprs_pass)) {
    debugBlink(4, 500, 500);
    Serial.println(" ОШИБКА: не удалось подключиться.");
    return false;
  }

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
    debugBlink(5, 500, 500);
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
 
  // FIXME: как отладишь часть с MQTT выпилить отсюда
  //return processServerResponse(response_body);
  return true;
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

  // if (!ensureGprsConnection()) {
  //   return false;
  // }

  debugBlink(5, 100, 100);
     

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



// bool attemptToSend(String messageText, uint32_t failCounter) {
//   String payloadToSend = prepareLogPayload();

//   bool wasSent = sendPayloadWithFallback(payloadToSend, messageText);

//   if (wasSent) {
//     LittleFS.remove(LOG_FILE_PATH);
//     return true;
//   }

//   // если связь совсем плохая, пробуем смс отправить с самой важной информацией
//   if (failCounter >= MAX_FAILS_SEND_COUNT) {
//     String shortMsg = "Send error" + getPowerControlledSensorsData();
//     sendSms(readStringSetting(SETTING_PHONE, DEFAULT_PHONE_NUMBER), shortMsg);
//   }

//   return false;
// }






















/**
 * @brief Подключается к MQTT, чтобы получить сохраненное сообщение с настройками.
 * @return true, если настройки были получены и обработаны.
 */
bool fetchMqttSettings() {
  String host = readStringSetting(SETTING_MQTT_HOST, DEFAULT_MQTT_HOST);
  int port = readIntSetting(SETTING_MQTT_PORT, DEFAULT_MQTT_PORT);
  String user = readStringSetting(SETTING_MQTT_USER, DEFAULT_MQTT_USER);
  String pass = readStringSetting(SETTING_MQTT_PASS, DEFAULT_MQTT_PASS);
  int devId = DEFAULT_DEVICE_ID;

  // Сбрасываем флаг перед каждой попыткой
  settingsReceived = false;

  // Устанавливаем сервер и нашу специальную callback-функцию
  mqttClient.setServer(host.c_str(), port);
  mqttClient.setCallback(settingsCallback);

  String clientId = "device-fetch-" + String(devId) + "-" + String(random(0xffff), HEX);
  if (!mqttClient.connect(clientId.c_str(), user.c_str(), pass.c_str())) {
    Serial.println("ОШИБКА: Не удалось подключиться для получения настроек.");
    return false;
  }

  String settingsTopic = "devices/config/" + String(devId);
  Serial.print("Подписались на топик настроек: ");
  Serial.println(settingsTopic);
  mqttClient.subscribe(settingsTopic.c_str());

  // --- КЛЮЧЕВАЯ ЛОГИКА: ЖДЕМ ОТВЕТА КОРОТКОЕ ВРЕМЯ ---
  Serial.println("Ждем ответа с настройками (таймаут 5 секунд)...");
  unsigned long startTime = millis();
  while (!settingsReceived && millis() - startTime < 5000) {
    // В этом цикле мы активно "слушаем" эфир
    mqttClient.loop();
    delay(10); // Небольшая пауза, чтобы не загружать процессор
  }

  // Отключаемся в любом случае
  mqttClient.disconnect();

  if (settingsReceived) {
    Serial.println("Получение настроек завершено успешно.");
  } else {
    Serial.println("Таймаут. Новых настроек нет.");
  }

  return settingsReceived;
}





/**
 * @brief Конвертирует строку формата "DD.MM.YYYY HH:MM:SS" в Unix Timestamp.
 * @param dateTimeStr Входная строка с датой и временем.
 * @return Unix Timestamp (uint64_t) или 0 в случае ошибки.
 */
uint64_t stringToTimestamp(const String& dateTimeStr) {
  struct tm tm;
  // Используем sscanf для эффективного разбора строки по формату
  // %d - целое число
  int items_scanned = sscanf(dateTimeStr.c_str(), "%d.%d.%d %d:%d:%d",
                             &tm.tm_mday, &tm.tm_mon, &tm.tm_year,
                             &tm.tm_hour, &tm.tm_min, &tm.tm_sec);

  // Проверяем, что все 6 полей были успешно разобраны
  if (items_scanned != 6) {
    Serial.print("ОШИБКА: Не удалось разобрать строку с датой: ");
    Serial.println(dateTimeStr);
    return 0;
  }

  // Корректируем значения для структуры tm
  tm.tm_year -= 1900; // Годы в struct tm считаются от 1900
  tm.tm_mon -= 1;     // Месяцы в struct tm считаются от 0 (0=Январь)

  // Устанавливаем часовой пояс в UTC, чтобы избежать проблем с локальным временем
  setenv("TZ", "UTC", 1);
  tzset();

  // Конвертируем структуру tm в Unix Timestamp
  time_t timestamp = mktime(&tm);

  return (uint64_t)timestamp;
}



/**
 * @brief Заполняет Protobuf-структуру, разбирая готовую CSV-строку.
 * @param csv Входная CSV-строка с данными сенсоров.
 * @param message Указатель на Protobuf-структуру для заполнения.
 */
void parseCsvAndFillProtobuf(const String& csv, iot_telemetry_TelemetryData* message) {
  // ВАЖНО: Порядок полей здесь должен ТОЧНО соответствовать порядку в CSV!
  
  message->boot_counter = getNthValue(csv, 0).toInt();

  message->time_start = stringToTimestamp(getNthValue(csv, 1));
  message->time_send = message->time_start;

  message->temperature_bme280 = getNthValue(csv, 2).toFloat();
  message->humidity           = getNthValue(csv, 3).toFloat();
  message->pressure           = getNthValue(csv, 4).toFloat();
  message->temperature_rtc    = getNthValue(csv, 5).toFloat();
  message->water_temperature  = getNthValue(csv, 6).toFloat();

  message->water_level        = getNthValue(csv, 12).toFloat();
  message->u_battery          = getNthValue(csv, 13).toFloat();
  message->load_current          = getNthValue(csv, 14).toFloat();
  message->load_power          = getNthValue(csv, 15).toFloat();

  
  //TODO: дописать по остальным параметрам как их получить
}



/**
 * @brief Собирает все данные и упаковывает их в бинарный Protobuf-пакет.
 *        (НОВАЯ ВЕРСИЯ: принимает готовую CSV-строку)
 * @param csv_payload Готовая CSV-строка с данными сенсоров.
 * @param buffer Буфер для записи бинарных данных.
 * @param buffer_size Размер буфера.
 * @return Количество байт, записанных в буфер, или 0 в случае ошибки.
 */
size_t prepareProtobufPayload(const String& csv_payload, uint8_t* buffer, size_t buffer_size) {
  // 1. Создаем структуру сообщения
  iot_telemetry_TelemetryData message = iot_telemetry_TelemetryData_init_zero;

  // 2. (НОВОЕ) Заполняем поля из CSV-строки
  parseCsvAndFillProtobuf(csv_payload, &message);

  // 3. Заполняем остальные поля, которых нет в CSV (системные)
  message.dev_id = 7001;
  message.gps_y = 0; // TODO: Добавить
  message.gps_x = 0; // TODO: Добавить
  message.time_mount = 0; // TODO: Добавить
  strncpy(message.ver, FIRMWARE_VERSION, sizeof(message.ver));

  // Данные о связи и питании
  message.quality = 27;//modem.getSignalQuality();
  strncpy(message.operator_name, modem.getOperator().c_str(), sizeof(message.operator_name));
  message.bat = modem.getBattPercent();
  message.u_modem = modem.getBattVoltage() / 1000.0f; // Конвертируем из мВ в В
  strncpy(message.imei, modem.getIMEI().c_str(), sizeof(message.imei));

  int year, month, day, hour, min, sec;
  float timezone;
  if (modem.getNetworkTime(&year, &month, &day, &hour, &min, &sec, &timezone)) {
      // TODO: Конвертировать в Unix Timestamp и записать в message.operator_time
  }

  // 4. Кодируем (сериализуем) сообщение в бинарный формат
  pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);
  bool status = pb_encode(&stream, iot_telemetry_TelemetryData_fields, &message);

  if (!status) {
    Serial.println("ОШИБКА: Не удалось закодировать Protobuf сообщение!");
    return 0;
  }

  Serial.print("Protobuf сообщение успешно создано, размер: ");
  Serial.println(stream.bytes_written);

  return stream.bytes_written;
}



/**
 * @brief Отправляет бинарное сообщение на MQTT-брокер.
 * @param payload Указатель на буфер с данными.
 * @param length Длина данных в буфере.
 * @return true в случае успеха, иначе false.
 */
bool sendMqttMessage(uint8_t* payload, unsigned int length) {
  String host = readStringSetting(SETTING_MQTT_HOST, DEFAULT_MQTT_HOST);
  int port = readIntSetting(SETTING_MQTT_PORT, DEFAULT_MQTT_PORT);
  String user = readStringSetting(SETTING_MQTT_USER, DEFAULT_MQTT_USER);
  String pass = readStringSetting(SETTING_MQTT_PASS, DEFAULT_MQTT_PASS);
  int devId = readIntSetting(SETTING_DEVICE_ID, DEFAULT_DEVICE_ID);


  mqttClient.setServer(host.c_str(), port);

  Serial.print("Подключение к MQTT-брокеру: ");
  Serial.println(host);

  // Формируем уникальный ClientID и топик
  String clientId = "device-" + String(devId) + "-" + String(random(0xffff), HEX);
  String topic = "devices/telemetry/" + String(devId);

  if (!mqttClient.connect(clientId.c_str(), user.c_str(), pass.c_str())) {
    Serial.print("ОШИБКА MQTT: не удалось подключиться, код: ");
    Serial.println(mqttClient.state());
    return false;
  }

  Serial.print("Успешно подключено. Публикация в топик: ");
  Serial.println(topic);

  // Публикуем бинарное сообщение
  if (mqttClient.publish(topic.c_str(), payload, length)) {
    Serial.println("Сообщение успешно опубликовано.");
    mqttClient.disconnect();
    return true;
  } else {
    Serial.println("ОШИБКА: Не удалось опубликовать сообщение.");
    mqttClient.disconnect();
    return false;
  }
}







/**
 * @brief Новая главная функция для отправки данных по MQTT.
 *        (НОВАЯ ВЕРСИЯ: принимает готовую CSV-строку)
 * @param csv_payload Готовая CSV-строка с данными сенсоров.
 * @return true, если данные были успешно отправлены.
 */
bool attemptToSendMqtt(const String& csv_payload) {
  if (!ensureGprsConnection()) {
    return false;
  }

  // Создаем буфер для Protobuf сообщения
  uint8_t buffer[iot_telemetry_TelemetryData_size];
  size_t message_length = prepareProtobufPayload(csv_payload, buffer, sizeof(buffer));

  if (message_length == 0) {
    return false; // Ошибка при создании сообщения
  }

  debugBlink(2, 200, 200);

  if (sendMqttMessage(buffer, message_length)) {

    debugBlink(3, 200, 200);

    fetchMqttSettings();
    return true;
  } else {
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

  int transportType = readIntSetting(SETTING_TRANSPORT_TYPE, TRANSPORT_HTTP);
  bool wasSent = false;

  // if (transportType == TRANSPORT_MQTT) {
  //   Serial.println("Выбран метод отправки: MQTT");
  //   wasSent = attemptToSendMqtt(messageText);
  // } else {
  //   Serial.println("Выбран метод отправки: HTTP");
  //   String payloadToSend = prepareLogPayload();
  //   wasSent = sendPayloadWithFallback(payloadToSend, messageText);
  // }

  // TODO: по МКТТ не отправляется история накопленная если она есть

  // пока двумя способами пробуем отправить


  attemptToSendMqtt(messageText);

  String payloadToSend = prepareLogPayload();
  wasSent = sendPayloadWithFallback(payloadToSend, messageText);

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




