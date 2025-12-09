package main

import (
    "encoding/json"
    "fmt"
    "log"
    "os"
    "time"

    mqtt "github.com/eclipse/paho.mqtt.golang"
)

// --- НАСТРОЙКИ MQTT-БРОКЕРА (измените, если нужно) ---
const (
    mqttBroker = "localhost"
    mqttPort   = 1883
    mqttUser   = "device_user" // Если у вас есть аутентификация
    mqttPass   = "sm191DY1oN5TDxMz" // Если у вас есть аутентификация
    clientID   = "config-publisher-service"
    settingsFile = "settings.json"
)

// Структура для хранения настроек одного устройства (должна совпадать с JSON)
type DeviceSettings struct {
    ModemActivationFrequency int     `json:"MODEM_ACTIVATION_FREQENCY"`
    CurrentMinMA             float64 `json:"CURRENT_MIN_MA"`
    ResistorOhms             float64 `json:"RESISTOR_OHMS"`
    PhoneNumber              string  `json:"PHONE_NUMBER"`
}

func main() {
    // Настраиваем MQTT-клиента
    opts := mqtt.NewClientOptions()
    opts.AddBroker(fmt.Sprintf("tcp://%s:%d", mqttBroker, mqttPort))
    opts.SetClientID(clientID)
    opts.SetUsername(mqttUser)
    opts.SetPassword(mqttPass)
    opts.SetAutoReconnect(true)
    opts.SetConnectRetry(true)

    client := mqtt.NewClient(opts)
    if token := client.Connect(); token.Wait() && token.Error() != nil {
        log.Fatalf("Не удалось подключиться к MQTT: %v", token.Error())
    }
    log.Println("Успешно подключено к MQTT-брокеру.")

    // Бесконечный цикл для постоянной публикации
    for {
        // 1. Читаем файл с настройками
        fileBytes, err := os.ReadFile(settingsFile)
        if err != nil {
            log.Printf("ОШИБКА: Не удалось прочитать файл %s: %v", settingsFile, err)
            time.Sleep(5 * time.Second) // Ждем 5 секунд перед повторной попыткой
            continue
        }

        // 2. Разбираем JSON в карту, где ключ - ID устройства, значение - его настройки
        var allSettings map[string]DeviceSettings
        if err := json.Unmarshal(fileBytes, &allSettings); err != nil {
            log.Printf("ОШИБКА: Не удалось разобрать JSON из файла %s: %v", settingsFile, err)
            time.Sleep(5 * time.Second)
            continue
        }

        // 3. Проходим по каждому устройству в файле и публикуем его настройки
        for deviceID, settings := range allSettings {
              
            // Создаем гибкую карту для добавления динамических полей
            payloadMap := make(map[string]interface{})
            payloadMap["MODEM_ACTIVATION_FREQENCY"] = settings.ModemActivationFrequency
            payloadMap["CURRENT_MIN_MA"] = settings.CurrentMinMA
            payloadMap["RESISTOR_OHMS"] = settings.ResistorOhms
            payloadMap["PHONE_NUMBER"] = settings.PhoneNumber

            // Добавляем текущее время сервера в нужном формате (ISO 8601 без долей секунд)
            // time.RFC3339 - это "2006-01-02T15:04:05Z07:00". Нам нужен формат без часового пояса.
            // Используем кастомный layout, чтобы точно соответствовать Python-версии.
            serverTime := time.Now().Format("2006-01-02T15:04:05")
            payloadMap["current_server_time"] = serverTime

            // Превращаем нашу расширенную карту в JSON для отправки
            payload, err := json.Marshal(payloadMap)
            if err != nil {
                log.Printf("ОШИБКА: Не удалось создать JSON для устройства %s: %v", deviceID, err)
                continue
            }


            // Формируем топик для этого устройства
            topic := fmt.Sprintf("devices/config/%s", deviceID)

            // Публикуем сообщение с флагом retain=true!
            // Это КЛЮЧЕВОЙ МОМЕНТ. Брокер сохранит это сообщение для новых подписчиков.
            token := client.Publish(topic, 1, true, payload)
            token.Wait() // Ждем завершения публикации

            if token.Error() != nil {
                log.Printf("ОШИБКА: Не удалось опубликовать настройки для устройства %s: %v", deviceID, token.Error())
            } else {
                log.Printf("Настройки для устройства %s успешно опубликованы в топик %s", deviceID, topic)
            }
        }

        // Ждем 1 секунду перед следующим циклом.
        // В реальной системе этот интервал можно сделать гораздо больше (например, 1 минута).
        log.Println("--- Пауза 1 секунда ---")
        time.Sleep(1 * time.Second)
    }
}
