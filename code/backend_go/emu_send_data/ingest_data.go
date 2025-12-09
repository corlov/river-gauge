// backend_service.go
package main

import (
	"log"
	"os"
	"os/signal"
	"syscall"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"google.golang.org/protobuf/proto"

	// Снова используем наш сгенерированный пакет
	"emu_device/telemetry" // Убедитесь, что имя модуля совпадает с вашим go.mod
)

// --- Настройки те же, что и у эмулятора ---
const (
    mqttBroker   = "tcp://89.169.3.241:1883"
    mqttTopic    = "devices/telemetry/+" // Используем '+' как wildcard для ID любого устройства
    mqttUser     = "device_user"
    mqttPassword = "sm191DY1oN5TDxMz"
    mqttClientID = "go-backend-service"
)

// messageHandler - это функция, которая будет вызываться каждый раз,
// когда в топик приходит новое сообщение.
var messageHandler mqtt.MessageHandler = func(client mqtt.Client, msg mqtt.Message) {
    log.Printf("Получено сообщение в топик: %s", msg.Topic())

    // 1. Создаем пустую структуру для наших данных
    telemetryData := &telemetry.TelemetryData{}

    // 2. Десериализуем бинарные данные из сообщения в нашу структуру
    // Это обратная операция к proto.Marshal()
    err := proto.Unmarshal(msg.Payload(), telemetryData)
    if err != nil {
        log.Printf("ОШИБКА: не удалось десериализовать Protobuf: %v", err)
        return
    }

    // 3. Теперь у нас есть доступ ко всем полям!
    log.Printf("Данные от устройства ID %d:", telemetryData.DevId)
    log.Printf("  - Температура BME: %.2f °C", telemetryData.TemperatureBme280)
    log.Printf("  - Уровень воды: %.2f м", telemetryData.WaterLevel)
    log.Printf("  - Напряжение АКБ: %.2f В", telemetryData.UBattery)
    log.Printf("  - Версия прошивки: %s", telemetryData.Ver)
}

func main() {
    opts := mqtt.NewClientOptions()
    opts.AddBroker(mqttBroker)
    opts.SetClientID(mqttClientID)
    opts.SetUsername(mqttUser)
    opts.SetPassword(mqttPassword)

	opts.SetCleanSession(false)

    // Устанавливаем обработчик по умолчанию
    opts.SetDefaultPublishHandler(messageHandler)

    // Устанавливаем обработчик для события подключения
    opts.OnConnect = func(c mqtt.Client) {
        log.Println("Подключено к MQTT-брокеру.")
        // Подписываемся на топик после успешного подключения
        if token := c.Subscribe(mqttTopic, 1, nil); token.Wait() && token.Error() != nil {
            log.Fatalf("Ошибка подписки на топик: %v", token.Error())
        }
        log.Printf("Успешно подписан на топик: %s", mqttTopic)
    }

    client := mqtt.NewClient(opts)
    if token := client.Connect(); token.Wait() && token.Error() != nil {
        log.Fatalf("Ошибка подключения к MQTT: %v", token.Error())
    }

    // Бесконечный цикл, чтобы программа не завершалась
    // и продолжала слушать сообщения.
    log.Println("Сервис запущен. Ожидание сообщений... (Нажмите Ctrl+C для выхода)")
    sig := make(chan os.Signal, 1)
    signal.Notify(sig, syscall.SIGINT, syscall.SIGTERM)
    <-sig

    log.Println("Получен сигнал завершения. Отключаемся...")
    client.Disconnect(250)
    log.Println("Сервис остановлен.")
}
