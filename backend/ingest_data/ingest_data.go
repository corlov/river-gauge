package main

import (
    "fmt"
    "log"
    "os"
    "os/signal"
    "syscall"
    "time"
    
    mqtt "github.com/eclipse/paho.mqtt.golang"
    influxdb2 "github.com/influxdata/influxdb-client-go/v2"
    "github.com/influxdata/influxdb-client-go/v2/api"
    "google.golang.org/protobuf/proto"
    "ingest_data/telemetry"
)




const (
    mqttBroker   = "tcp://localhost:1883"
    mqttTopic    = "devices/telemetry/+"
    mqttUser     = "device_user"
    mqttPassword = "sm191DY1oN5TDxMz"
    mqttClientID = "go-backend-service-persistent"
)




const (
    influxURL    = "http://localhost:8086"
    influxToken  = "LwyGVsg57PLcO14HUVIqnUj3HPUGF4nl9Lc5kHJRhGMiHefFec_8BCqUKxZ6mDOyFjUN50VsKtORjUpvT4hsmQ=="
    influxOrg    = "WW"
    influxBucket = "WW"
)


var influxWriteAPI api.WriteAPI


var messageHandler mqtt.MessageHandler = func(client mqtt.Client, msg mqtt.Message) {
    log.Printf("Получено сообщение в топик: %s", msg.Topic())
    
    telemetryData := &telemetry.TelemetryData{}
    err := proto.Unmarshal(msg.Payload(), telemetryData)
    if err != nil {
        log.Printf("ОШИБКА: не удалось десериализовать Protobuf: %v", err)
        return
    }

    log.Printf("Данные от устройства ID %d: Уровень воды %.2f м, АКБ %.2f В",
    telemetryData.DevId, telemetryData.WaterLevel, telemetryData.UBattery)

    

    // Создаем новую "точку" данных для измерения "telemetry"
    p := influxdb2.NewPointWithMeasurement("telemetry").
        // Добавляем Теги (индексируемые метаданные)
        AddTag("device_id", fmt.Sprintf("%d", telemetryData.DevId)).
        AddTag("firmware_version", telemetryData.Ver).
        AddTag("operator", telemetryData.Operator).
        AddTag("imei", telemetryData.Imei).

        // Добавляем Поля (измеряемые значения)
        AddField("temperature_bme280", telemetryData.TemperatureBme280).
        AddField("humidity", telemetryData.Humidity).
        AddField("pressure", telemetryData.Pressure).
        AddField("temperature_rtc", telemetryData.TemperatureRtc).
        AddField("water_temperature", telemetryData.WaterTemperature).
        AddField("water_level", telemetryData.WaterLevel).
        AddField("u_battery", telemetryData.UBattery).
        AddField("load_current", telemetryData.LoadCurrent).
        AddField("load_power", telemetryData.LoadPower).
        AddField("gps_y", telemetryData.GpsY).
        AddField("gps_x", telemetryData.GpsX).
        AddField("signal_quality", telemetryData.Quality).
        AddField("battery_percent", telemetryData.Bat).
        AddField("u_modem", telemetryData.UModem).
        AddField("boot_counter", telemetryData.BootCounter).

        // Устанавливаем время для этой точки данных. Берем его из сообщения.
        SetTime(time.Unix(int64(telemetryData.TimeSend), 0))

    // Асинхронно записываем точку. Клиент сам накапливает их и отправляет пачками.
    influxWriteAPI.WritePoint(p)
}




func main() {

    influxClient := influxdb2.NewClient(influxURL, influxToken)
    // Получаем API для записи данных
    influxWriteAPI = influxClient.WriteAPI(influxOrg, influxBucket)
    // Получаем канал для ошибок записи
    errorsCh := influxWriteAPI.Errors()
    // Запускаем горутину для отслеживания ошибок в фоне
    go func() {
        for err := range errorsCh {
            log.Printf("Ошибка записи в InfluxDB: %s\n", err.Error())
        }
    }()
        
    opts := mqtt.NewClientOptions()
    opts.AddBroker(mqttBroker)
    opts.SetClientID(mqttClientID)
    opts.SetUsername(mqttUser)
    opts.SetPassword(mqttPassword)
    opts.SetCleanSession(false)
    opts.SetDefaultPublishHandler(messageHandler)
    opts.OnConnect = func(c mqtt.Client) {
        log.Println("Подключено к MQTT-брокеру.")
        if token := c.Subscribe(mqttTopic, 1, nil); token.Wait() && token.Error() != nil {
            log.Fatalf("Ошибка подписки на топик: %v", token.Error())
        }
        log.Printf("Успешно подписан на топик: %s", mqttTopic)
    }

    client := mqtt.NewClient(opts)
    if token := client.Connect(); token.Wait() && token.Error() != nil {
        log.Fatalf("Ошибка подключения к MQTT: %v", token.Error())
    }

    log.Println("Сервис запущен. Ожидание сообщений... (Нажмите Ctrl+C для выхода)")
    sig := make(chan os.Signal, 1)
    signal.Notify(sig, syscall.SIGINT, syscall.SIGTERM)
    <-sig

    log.Println("Получен сигнал завершения. Отключаемся...")


    influxWriteAPI.Flush()
    influxClient.Close()
    client.Disconnect(250)
    log.Println("Сервис остановлен.")
}

