#include "broker.h"
#include "globals.h"
#include "water_lvl_settings.h"
#include "sensors.h"
#include "process_response.h"
#include <time.h> 
#include <PubSubClient.h>
#include "water_lvl_utils.h"
#include "water_lvl_init.h"
#include "storage.h"
#include "errors.h"


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

    settingsReceived = false;

    String clientId = "device-fetch-" + String(devId) + "-" + String(random(0xffff), HEX);

    // TODO: предусмотреть подключение к резервному серверу
    mqttClient.setServer(host.c_str(), port);
    mqttClient.setCallback(settingsCallback);

    if (!mqttClient.connect(clientId.c_str(), user.c_str(), pass.c_str())) {
        blinkErrorCode(ERR_MQTT_CONNECT);
        Serial.println("ОШИБКА: Не удалось подключиться для получения настроек.");
        return false;
    }

    String settingsTopic = "devices/config/" + String(devId);
    mqttClient.subscribe(settingsTopic.c_str());

    unsigned long startTime = millis();
    while (!settingsReceived && millis() - startTime < WAITING_SETTINGS_REQ_TIMEOUT) {
        mqttClient.loop();
        delay(20);
    }

    mqttClient.disconnect();
    return settingsReceived;
}



/**
 * @brief Заполняет Protobuf-структуру, разбирая готовую CSV-строку.
 * @param csv Входная CSV-строка с данными сенсоров.
 * @param message Указатель на Protobuf-структуру для заполнения.
 */
void parseCsvAndFillProtobuf(const String& csv, iot_telemetry_TelemetryData* message) {
    message->boot_counter = getNthValue(csv, 0).toInt();

    message->time_start = stringToTimestamp(getNthValue(csv, 1));
    message->time_send = stringToTimestamp(getDateTime());

    message->temperature_bme280 = getNthValue(csv, 2).toFloat();
    message->humidity           = getNthValue(csv, 3).toFloat();
    message->pressure           = getNthValue(csv, 4).toFloat();
    message->temperature_rtc    = getNthValue(csv, 5).toFloat();
    message->water_temperature  = getNthValue(csv, 6).toFloat();

    message->dev_id = getNthValue(csv, 7).toInt();
    message->gps_x  = GPS_LON;
    message->gps_y  = GPS_LAT;
    message->time_mount  = stringToTimestamp(getNthValue(csv, 11) + " 00:00:00");
    strncpy(message->ver, getNthValue(csv, 10).c_str(), sizeof(message->ver));

    message->water_level        = getNthValue(csv, 12).toFloat();
    message->u_battery          = getNthValue(csv, 13).toFloat();
    message->load_current       = getNthValue(csv, 14).toFloat();
    message->load_power         = getNthValue(csv, 15).toFloat();

    message->quality = modem.getSignalQuality(); 

    String operatorName = modem.getOperator();
    if (operatorName.isEmpty() || operatorName == "0" || operatorName.length() < 2) {
        strncpy(message->operator_name, "N/A", sizeof(message->operator_name));
    } else {
        strncpy(message->operator_name, operatorName.c_str(), sizeof(message->operator_name));
    }

    message->bat = modem.getBattPercent();
    message->u_modem = modem.getBattVoltage() / 1000.0f;

    String imei = modem.getIMEI();
    if (imei.isEmpty() || imei.length() < 15) {
        strncpy(message->imei, "N/A", sizeof(message->imei));
    } else {
        strncpy(message->imei, imei.c_str(), sizeof(message->imei));
    }

    int year, month, day, hour, min, sec;
    float timezone;
    if (modem.getNetworkTime(&year, &month, &day, &hour, &min, &sec, &timezone)) {
        uint64_t operatorTimestamp = convertToTimestamp(year, month, day, hour, min, sec);
        if (operatorTimestamp > 0) {
            message->operator_time = operatorTimestamp;
        } else {
            message->operator_time = 0;
        }
    }
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
  iot_telemetry_TelemetryData message = iot_telemetry_TelemetryData_init_zero;

  parseCsvAndFillProtobuf(csv_payload, &message);

  pb_ostream_t stream = pb_ostream_from_buffer(buffer, buffer_size);
  bool status = pb_encode(&stream, iot_telemetry_TelemetryData_fields, &message);

  if (!status) {
    return 0;
  }

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

    // TODO: подключение к резервному серверу
    mqttClient.setServer(host.c_str(), port);

    // Формируем уникальный ClientID и топик
    String clientId = "device-" + String(devId) + "-" + String(random(0xffff), HEX);
    String topic = "devices/telemetry/" + String(devId);

    if (!mqttClient.connect(clientId.c_str(), user.c_str(), pass.c_str())) {
        Serial.print("ОШИБКА MQTT: не удалось подключиться, код: ");
        Serial.println(mqttClient.state());
        blinkErrorCode(ERR_MQTT_CONNECT_2);
        return false;
    }

    if (mqttClient.publish(topic.c_str(), payload, length)) {
        mqttClient.disconnect();
        return true;
    } 

    mqttClient.disconnect();
    blinkErrorCode(ERR_PUBLISH_MSG);
    Serial.println("ОШИБКА: Не удалось опубликовать сообщение.");
    return false;
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

    // TODO: здесь нужно сделать цикл по ЦСВ-файлу что лежит в ПЗУ и передать все накопленое что есть, первым передаем свежую строку, заетм все остальное
    uint8_t buffer[iot_telemetry_TelemetryData_size];
    size_t message_length = prepareProtobufPayload(csv_payload, buffer, sizeof(buffer));

    if (message_length == 0) {
        return false;
    }

    if (sendMqttMessage(buffer, message_length)) {
        return fetchMqttSettings();
    } else {
        return false;
    }

    //TODO: fetchMqttSettings(); - вынести из цикла и после всех отправок получить
}
