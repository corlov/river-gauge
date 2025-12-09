#include <time.h> 
#include <PubSubClient.h>
#include "broker.h"
#include "globals.h"
#include "water_lvl_settings.h"
#include "water_lvl_types.h"
#include "sensors.h"
#include "process_response.h"
#include "water_lvl_utils.h"
#include "water_lvl_init.h"
#include "storage.h"
#include "errors.h"



/**
 * @brief Подключается к MQTT, чтобы получить сохраненное сообщение с настройками.
 * @return true, если настройки были получены и обработаны.
 */
bool fetchMqttSettings() {
    int devId = DEFAULT_DEVICE_ID;
    settingsReceived = false;

    String clientId = "device-fetch-" + String(devId) + "-" + String(random(0xffff), HEX);
    String settingsTopic = "devices/config/" + String(devId);

    mqttClient.setCallback(settingsCallback);

    for (const auto& config : serverConfigs) {

        mqttClient.setServer(config.host, config.port);

        if (mqttClient.connect(clientId.c_str(), config.user, config.pass)) {
            mqttClient.subscribe(settingsTopic.c_str());

            unsigned long startTime = millis();
            while (!settingsReceived && millis() - startTime < WAITING_SETTINGS_REQ_TIMEOUT) {
                mqttClient.loop();
                delay(20);
            }

            mqttClient.disconnect();
            debugBlink(3, 800, 200);
            return settingsReceived;
        } else {
            Serial.println("Не удалось подключиться. Пробую следующий сервер...");
        }
    }

    blinkErrorCode(ERR_MQTT_CONNECT);
    Serial.println("ОШИБКА: Не удалось подключиться ни к одному из доступных MQTT серверов.");
    return false;
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
 * Отправляет одно бинарное сообщение по MQTT.
 * Перебирает все серверы из глобального массива serverConfigs.
 * Для первого сервера (основного) настройки будут загружены из памяти.
 * @param payload Указатель на буфер с данными.
 * @param length Длина данных в буфере.
 * @return true в случае успешной отправки, иначе false.
 */
bool sendMqttMessage(uint8_t* payload, unsigned int length) {
    int devId = readIntSetting(SETTING_DEVICE_ID, DEFAULT_DEVICE_ID);
    String clientId = "device-" + String(devId) + "-" + String(random(0xffff), HEX);
    String topic = "devices/telemetry/" + String(devId);

    const size_t numServers = sizeof(serverConfigs) / sizeof(serverConfigs[0]);
    for (size_t i = 0; i < numServers; ++i) {
        String host = serverConfigs[i].host;
        int port = serverConfigs[i].port;
        String user = serverConfigs[i].user;
        String pass = serverConfigs[i].pass;

        mqttClient.setServer(host.c_str(), port);

        if (mqttClient.connect(clientId.c_str(), user.c_str(), pass.c_str())) {
            if (mqttClient.publish(topic.c_str(), payload, length)) {
                mqttClient.disconnect();
                return true;
            } else {
                blinkErrorCode(ERR_PUBLISH_MSG);
                Serial.println("ОШИБКА: Не удалось опубликовать сообщение.");
                mqttClient.disconnect();
                return false; // Ошибка публикации, нет смысла пробовать другие серверы.
            }
        }
    }

    blinkErrorCode(ERR_MQTT_CONNECT_2);
    Serial.println("ОШИБКА: Не удалось подключиться ни к одному из серверов для отправки данных.");
    return false;
}





/**
 * @brief функция для отправки данных по MQTT.
 * @param csv_payload Готовая CSV-строка с данными сенсоров.
 * @return true, если данные были успешно отправлены.
 */
bool attemptToSendMqtt(const String& csv_payload, const std::vector<String>& logLines) {
    if (!ensureGprsConnection()) {
        return false;
    }

    { // Используем блок для ограничения области видимости buffer и message_length
        uint8_t buffer[iot_telemetry_TelemetryData_size];
        size_t message_length = prepareProtobufPayload(csv_payload, buffer, sizeof(buffer));

        if (message_length == 0) {
            blinkErrorCode(ERR_PREPARE_PROTOBUF);
            Serial.println("Ошибка: не удалось подготовить Protobuf payload для свежей строки.");
            return false;
        }

        if (!sendMqttMessage(buffer, message_length)) {
            blinkErrorCode(ERR_SEND_PROTOBUF);
            Serial.println("Ошибка: не удалось отправить MQTT сообщение для свежей строки.");
            return false;
        }
    }
    debugBlink(8, 50, 50);

    // Отправка накопленных строк из лога
    for (const String& log_line : logLines) {
        if (log_line.length() == 0) continue;

        uint8_t buffer[iot_telemetry_TelemetryData_size];
        size_t message_length = prepareProtobufPayload(log_line, buffer, sizeof(buffer));

        if (message_length == 0) {
            blinkErrorCode(ERR_PREPARE_PROTOBUF);
            Serial.println("Ошибка: не удалось подготовить Protobuf payload для строки из лога. Отправка прервана.");
            return false;
        }

        if (!sendMqttMessage(buffer, message_length)) {
            blinkErrorCode(ERR_SEND_PROTOBUF);
            Serial.println("Ошибка: не удалось отправить MQTT сообщение для строки из лога. Отправка прервана.");
            return false;
        }
        delay(50);

        debugBlink(8, 50, 50);
    }

    return fetchMqttSettings();
}

