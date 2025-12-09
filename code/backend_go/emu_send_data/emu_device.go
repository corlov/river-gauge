package main

import (
	"log"
	"math/rand"
	"time"

	"emu_device/telemetry"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"google.golang.org/protobuf/proto"
)


const (
    mqttBroker   = "tcp://89.169.3.241:1883"
    mqttTopic    = "devices/telemetry/7001"
    mqttUser     = "device_user"
    mqttPassword = "sm191DY1oN5TDxMz"
    mqttClientID = "go-emulator-7001"
)


func main() {
    log.Println("Заполнение сообщения тестовыми данными...")

    msg := &telemetry.TelemetryData{
        // Системные и временные метрики
        BootCounter: 16,
        TimeStart:   uint64(time.Now().Unix()) - (rand.Uint64()*10 + 20),
        TimeSend:    uint64(time.Now().Unix()),

        // Данные с основных сенсоров
        TemperatureBme280: rand.Float32()*5 + 20,
        Humidity:          rand.Float32()*20 + 40,
        Pressure:          rand.Float32()*40 + 980,
        TemperatureRtc:    rand.Float32()*2 + 22,
        WaterTemperature:  rand.Float32()*5 + 10,
        WaterLevel:        rand.Float32()*3.5 + 5,
        UBattery:          rand.Float32()*0.7 + 12.1,
        LoadCurrent:       rand.Float32()*0.7 + 0.5,

        // Идентификационная информация
        DevId:      7001,
        GpsY:       56.065226,
        GpsX:       36.234294,
        TimeMount:  1764537600,
        Ver:        "1.0.1-go",

        // Данные о связи
        Quality:      -85,
        OperatorName:     "MegaFon",
        Bat:          98,
        UModem:       4.1,
        Imei:         "543210987654321",
        OperatorTime: uint64(time.Now().Unix() - 5),
    }
    msg.LoadPower = msg.UBattery * msg.LoadCurrent

    // --- Сериализация сообщения в бинарный формат ---
    payload, err := proto.Marshal(msg)
    if err != nil {
        log.Fatalf("Ошибка сериализации Protobuf: %v", err)
    }
    log.Printf("Сообщение сериализовано. Размер: %d байт.", len(payload))

    // --- Настройка и подключение MQTT-клиента ---
    opts := mqtt.NewClientOptions()
    opts.AddBroker(mqttBroker)
    opts.SetClientID(mqttClientID)
    opts.SetUsername(mqttUser)
    opts.SetPassword(mqttPassword)

    client := mqtt.NewClient(opts)
    log.Printf("Подключение к MQTT-брокеру %s...", mqttBroker)
    if token := client.Connect(); token.Wait() && token.Error() != nil {
        log.Fatalf("Ошибка подключения к MQTT: %v", token.Error())
    }

    // --- Публикация сообщения ---
    log.Printf("Публикация сообщения в топик '%s'...", mqttTopic)
    token := client.Publish(mqttTopic, 1, false, payload) // QoS=1, not retained
    token.Wait() // Ждем завершения отправки

    client.Disconnect(250) // Отключаемся
    log.Println("Сообщение успешно отправлено.")
}
