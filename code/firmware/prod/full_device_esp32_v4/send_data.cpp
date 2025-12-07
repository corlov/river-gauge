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
    return false;
  }

  if (!modem.waitForNetwork(30000L)) { // Увеличим таймаут ожидания сети до 30 сек
    Serial.println(" ОШИБКА: сеть не найдена.");
    return false;
  }

  if (!modem.gprsConnect(apn, gprs_user, gprs_pass)) {
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
 * @brief Возвращает текущее время в формате Unix Timestamp (секунды с 1970 г).
 * @return Текущий Unix Timestamp, или 0 если время не синхронизировано.
 */
uint64_t getUnixTime() {
  // if (!time_synced) {
  //   // Если время еще не синхронизировано, можно попробовать сделать это сейчас
  //   syncTimeWithNetwork();
  // }

  // time_t now;
  // time(&now);
  //return (uint64_t)now;

  return 0;
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


// /**
//  * @brief Собирает все данные и упаковывает их в бинарный Protobuf-пакет.
//  * @param buffer Буфер для записи бинарных данных.
//  * @param buffer_size Размер буфера.
//  * @return Количество байт, записанных в буфер, или 0 в случае ошибки.
//  */
// size_t prepareProtobufPayload(uint8_t* buffer, size_t buffer_size) {
//   // 1. Создаем структуру сообщения
//   iot_telemetry_TelemetryData message = iot_telemetry_TelemetryData_init_zero;

//   // 2. Создаем поток для записи в наш буфер
//   pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);

//   // 3. Заполняем поля сообщения данными с сенсоров и системы
//   // (Здесь нужно вызывать ваши функции для получения данных)

//   // Системные и временные метрики
//   message.boot_counter = 5;//FIXME bootCount; // Предполагается, что у вас есть такая переменная
//   message.time_start = 0; // TODO: Добавить время старта
//   message.time_send = getUnixTime(); // TODO: Нужна функция для получения Unix time

//   // Данные с основных сенсоров
//   message.temperature_bme280 = 36.6;//lastBME280Temp;
//   message.humidity = 1.0;//lastBME280Humidity;
//   message.pressure = 1.0;//lastBME280Pressure;
//   message.temperature_rtc = 1.0;//getRtcTemperature();
//   message.water_temperature = 1.0;//getWaterTemperature();
//   message.water_level = 1.0;//getWaterLevel();
//   message.u_battery = 1.0;//getBatteryVoltage();
//   message.load_current = 0; // TODO: Добавить
//   message.load_power = 0;   // TODO: Добавить

//   // Идентификационная информация
//   message.dev_id = 7001;
//   message.gps_y = 0; // TODO: Добавить
//   message.gps_x = 0; // TODO: Добавить
//   message.time_mount = 0; // TODO: Добавить
//   strncpy(message.ver, FIRMWARE_VERSION, sizeof(message.ver));

//   // Данные о связи и питании
//   message.quality = 27;//modem.getSignalQuality();
//   strncpy(message.operator_name, modem.getOperator().c_str(), sizeof(message.operator_name));
//   message.bat = modem.getBattPercent();
//   message.u_modem = modem.getBattVoltage() / 1000.0f; // Конвертируем из мВ в В
//   strncpy(message.imei, modem.getIMEI().c_str(), sizeof(message.imei));

//   int year, month, day, hour, min, sec;
//   float timezone;
//   if (modem.getNetworkTime(&year, &month, &day, &hour, &min, &sec, &timezone)) {
//       // TODO: Конвертировать в Unix Timestamp и записать в message.operator_time
//   }

//   // 4. Кодируем (сериализуем) сообщение в бинарный формат
//   bool status = pb_encode(&stream, iot_telemetry_TelemetryData_fields, &message);

//   if (!status) {
//     Serial.println("ОШИБКА: Не удалось закодировать Protobuf сообщение!");
//     return 0;
//   }

//   Serial.print("Protobuf сообщение успешно создано, размер: ");
//   Serial.println(stream.bytes_written);

//   return stream.bytes_written;
// }


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

  return sendMqttMessage(buffer, message_length);
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




