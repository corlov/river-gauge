#!/usr/bin/python3
import paho.mqtt.client as mqtt
import time
import random
import telemetry_pb2


MQTT_BROKER = "89.169.3.241"
MQTT_PORT = 1883
MQTT_TOPIC = "devices/telemetry/7001"
MQTT_USER = "device_user"
MQTT_PASSWORD = "sm191DY1oN5TDxMz"


telemetry_message = telemetry_pb2.TelemetryData()

print("Заполнение сообщения тестовыми данными...")

# Системные и временные метрики
telemetry_message.boot_counter = 15
telemetry_message.time_start = int(time.time()) - 25
telemetry_message.time_send = int(time.time())

# Данные с основных сенсоров (генерируем случайные значения)
telemetry_message.temperature_bme280 = round(random.uniform(20.0, 25.0), 2)
telemetry_message.humidity = round(random.uniform(40.0, 60.0), 2)
telemetry_message.pressure = round(random.uniform(980.0, 1020.0), 2)
telemetry_message.temperature_rtc = round(random.uniform(22.0, 24.0), 2)
telemetry_message.water_temperature = round(random.uniform(10.0, 15.0), 2)
telemetry_message.water_level = round(random.uniform(5.0, 8.5), 2)
telemetry_message.u_battery = round(random.uniform(12.1, 12.8), 2)
telemetry_message.load_current = round(random.uniform(0.5, 1.2), 2)
telemetry_message.load_power = telemetry_message.u_battery * telemetry_message.load_current

# Идентификационная информация
telemetry_message.dev_id = 7001
telemetry_message.gps_y = 56.065226
telemetry_message.gps_x = 36.234294
telemetry_message.time_mount = 1764537600
telemetry_message.ver = "1.0.0"

# Данные о связи
telemetry_message.quality = 27
telemetry_message.operator = "MegaFon"
telemetry_message.bat = 95
telemetry_message.u_modem = 4.1
telemetry_message.imei = "123456789012345"
telemetry_message.operator_time = int(time.time()) - 5

# --- Сериализация сообщения ---
# Превращаем объект Python в бинарную строку Protobuf
payload = telemetry_message.SerializeToString()
print(f"Сообщение сериализовано. Размер: {len(payload)} байт.")

# --- Отправка по MQTT ---
try:
    client = mqtt.Client()
    client.username_pw_set(MQTT_USER, MQTT_PASSWORD)

    print(f"Подключение к MQTT-брокеру {MQTT_BROKER}...")
    client.connect(MQTT_BROKER, MQTT_PORT, 60)

    print(f"Публикация сообщения в топик '{MQTT_TOPIC}'...")
    client.publish(MQTT_TOPIC, payload, qos=1) # qos=1 для гарантированной доставки

    client.disconnect()
    print("Сообщение успешно отправлено.")

except Exception as e:
    print(f"ОШИБКА: Не удалось отправить сообщение. {e}")
