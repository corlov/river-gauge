#include "send_data.h"
#include "globals.h"
#include "water_lvl_settings.h"
#include "storage.h"
#include "LittleFS.h"



// --- Функция отправки: Принимает готовый пакет и отправляет его ---
// Возвращает true в случае успеха, false - в случае любой ошибки.
bool sendPayloadToServer(const String& payload) {
  //TODO: здесь надо начать измерение времени сколько ищем сеть
  if (payload.length() == 0) {
    Serial.println("Пакет для отправки пуст, отмена.");
    return false;
  }

  SerialGSM.begin(9600, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
  Serial.println("Инициализация модема...");
  if (!modem.restart()) {
    Serial.println("Не удалось перезагрузить модем!");
    return false;
  }
  Serial.println("Модем готов.");

  
  Serial.print("Ожидание сети...");
  if (!modem.waitForNetwork()) {
    Serial.println(" сеть не найдена.");
    return false;
  }
  Serial.println(" сеть найдена.");

  Serial.print("Подключение к GPRS...");
  if (!modem.gprsConnect(apn, gprs_user, gprs_pass)) {
    Serial.println(" не удалось подключиться.");
    return false;
  }
  Serial.println(" GPRS подключен.");

  
  bool success = false;
  Serial.print("Отправка данных на сервер ");
  Serial.print(receiver_server_address);
  Serial.print(":");
  Serial.println(receiver_server_port);

  if (client.connect(receiver_server_address, receiver_server_port)) {
    //TODO: здесь надо заканчивать измерение времени сколько искали сеть и добавлять к посылке - проблема в том что строки в посылке уже сформированы
    // поэтому это должен быть какой-то другой формат строки содержащий параметры технические

    // сколько секунд искалась связь - только что замерили
    // качество сигнала - узнать как его получать
    // попытка с которой это сообщение передано - это считаем значение из счетчика ошибок

    Serial.println("Соединение с сервером установлено.");

    client.print(String("POST / HTTP/1.1\r\n"));
    client.print(String("Host: ") + receiver_server_address + "\r\n");
    client.print("Connection: close\r\n");
    client.print("Content-Type: text/csv\r\n");
    client.print(String("Content-Length: ") + payload.length() + "\r\n");
    client.print("\r\n");
    client.print(payload);

    Serial.println("Пакет данных отправлен.");
    // ВАЖНО: Мы считаем отправку успешной, как только данные ушли в сокет.
    // Ответ сервера можно игнорировать или просто выводить для отладки.
    success = true;

    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 5000L) {
      while (client.available()) {
        Serial.write(client.read());
      }
    }
    Serial.println();

    client.stop();
    Serial.println("Соединение закрыто.");
  } else {
    Serial.println("Не удалось подключиться к серверу.");
    success = false;
  }

  modem.gprsDisconnect();
  Serial.println("GPRS отключен.");

  return success;
}



// Готовит пакет, отправляет его и, в случае успеха, очищает ВЕСЬ лог.
bool attemptToSendLogs() {
  Serial.println("\n--- Попытка отправки накопленных логов ---");

  String payloadToSend = prepareLogPayload();

  if (payloadToSend.isEmpty()) {
    Serial.println("Очередь логов пуста. Нечего отправлять.");
    return false;
  }

  bool wasSent = sendPayloadToServer(payloadToSend);
  if (wasSent) {
    Serial.println("УСПЕХ: Самые свежие данные были отправлены. Очищаю весь файл логов.");
    LittleFS.remove(LOG_FILE_PATH);

    // TODO: обнуляем счетчик ошибок
    return true;
  }

  Serial.println("ОШИБКА: Не удалось отправить данные. Файл логов сохранен для следующей попытки.");

  // TODO: увеличиваем счетчик ошибок
  return false;
}