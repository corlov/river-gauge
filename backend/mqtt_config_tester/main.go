package main

import (
	"fmt"
	"log"
	"os"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)


const (
    mqttBroker = "89.169.3.241" 
    mqttPort   = 1883
    mqttUser   = "device_user"
    mqttPass   = "sm191DY1oN5TDxMz"
)

func main() {
    if len(os.Args) < 2 {
        log.Fatalf("Использование: go run main.go <device_id>")
    }
    deviceID := os.Args[1]

    // 2. Создаем канал, который будет сигналом о получении сообщения
    // Это нужно, чтобы программа не завершилась раньше времени
    msgReceived := make(chan bool, 1)

    // 3. Настраиваем MQTT-клиента
    opts := mqtt.NewClientOptions()
    opts.AddBroker(fmt.Sprintf("tcp://%s:%d", mqttBroker, mqttPort))
    // Генерируем уникальный ClientID для теста
    opts.SetClientID(fmt.Sprintf("local-test-client-%s-%d", deviceID, time.Now().Unix()))
    opts.SetUsername(mqttUser)
    opts.SetPassword(mqttPass)

    // 4. (КЛЮЧЕВОЙ МОМЕНТ) Устанавливаем обработчик сообщений (callback)
    // Эта функция будет вызвана, когда придет сообщение
    opts.SetDefaultPublishHandler(func(client mqtt.Client, msg mqtt.Message) {
        log.Printf("--- ПОЛУЧЕНО СООБЩЕНИЕ ---")
        log.Printf("Топик: %s", msg.Topic())
        log.Printf("Содержимое: %s", string(msg.Payload()))
        log.Println("-------------------------")

        // Отправляем сигнал в наш канал, что сообщение получено
        msgReceived <- true
    })

    // 5. Подключаемся к брокеру
    client := mqtt.NewClient(opts)
    if token := client.Connect(); token.Wait() && token.Error() != nil {
        log.Fatalf("Не удалось подключиться к MQTT: %v", token.Error())
    }
    log.Println("Успешно подключено к MQTT-брокеру.")

    // 6. Формируем топик и подписываемся на него
    topic := fmt.Sprintf("devices/config/%s", deviceID)
    // QoS 1 - гарантированная доставка хотя бы один раз
    if token := client.Subscribe(topic, 1, nil); token.Wait() && token.Error() != nil {
        log.Fatalf("Не удалось подписаться на топик %s: %v", topic, token.Error())
    }
    log.Printf("Успешно подписались на топик: %s", topic)
    log.Println("Ожидание сохраненного сообщения (retained message)...")

    // 7. Ждем сигнала из канала, но не дольше 5 секунд
    select {
    case <-msgReceived:
        log.Println("Сообщение успешно получено.")
    case <-time.After(5 * time.Second):
        log.Println("Таймаут: сообщение не было получено. Убедитесь, что для этого топика есть сохраненное сообщение (retained message).")
    }

    // 8. Отключаемся и завершаем работу
    client.Disconnect(250)
    log.Println("Отключились от брокера. Тест завершен.")
}
