#include "send_data.h"
#include "globals.h"
#include "water_lvl_settings.h"
#include "storage.h"
#include "LittleFS.h"



bool sendSms(const String& phoneNumber, const String& message) {
  if (phoneNumber.length() < 10) {
    Serial.println("ОШИБКА SMS: Некорректный номер телефона.");
    return false;
  }
  if (message.length() == 0) {
    Serial.println("ОШИБКА SMS: Пустое сообщение.");
    return false;
  }

  Serial.print("Отправка SMS на номер: ");
  Serial.println(phoneNumber);
  Serial.print("Текст: ");
  Serial.println(message);

  // modem.sendSMS() возвращает true, если модуль подтвердил отправку.
  if (modem.sendSMS(phoneNumber, message)) {
    Serial.println("SMS успешно отправлена.");
    return true;
  } else {
    Serial.println("ОШИБКА: Не удалось отправить SMS.");
    return false;
  }
}


// --- Улучшенная функция отправки данных ---
// Возвращает true, только если сервер подтвердил получение данных.
bool sendPayloadToServer(const String& payload) {
  if (payload.length() == 0) {
    Serial.println("ОШИБКА: Пакет для отправки пуст, отмена.");
    return false;
  }

  // --- Шаг 1: Инициализация модема и подключение к сети ---
  // Примечание: В идеале, эту часть нужно вынести в setup() и переподключаться
  // только при необходимости, а не при каждой отправке.
  SerialGSM.begin(9600, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
  Serial.println("Инициализация модема...");
  if (!modem.restart()) {
    Serial.println("ОШИБКА: Не удалось перезагрузить модем!");
    return false;
  }
  Serial.println("Модем готов.");

  Serial.print("Ожидание сети...");
  if (!modem.waitForNetwork(30000L)) { // Увеличим таймаут ожидания сети до 30 сек
    Serial.println(" ОШИБКА: сеть не найдена.");
    return false;
  }
  Serial.println(" сеть найдена.");

  Serial.print("Подключение к GPRS...");
  if (!modem.gprsConnect(apn, gprs_user, gprs_pass)) {
    Serial.println(" ОШИБКА: не удалось подключиться.");
    return false;
  }
  Serial.println(" GPRS подключен.");

  // --- Шаг 2: Подключение к серверу и отправка данных ---
  bool success = false;
  Serial.print("Подключение к серверу ");
  Serial.print(receiver_server_address);
  Serial.print(":");
  Serial.println(receiver_server_port);

  // Увеличим таймаут подключения к серверу до 15 секунд
  if (client.connect(receiver_server_address, receiver_server_port, 15000L)) {
    Serial.println("Соединение с сервером установлено.");

    // Формируем и отправляем HTTP-запрос
    client.print(String("POST / HTTP/1.1\r\n"));
    client.print(String("Host: ") + receiver_server_address + "\r\n");
    client.print("Connection: close\r\n");
    client.print("Content-Type: text/csv\r\n");
    client.print(String("Content-Length: ") + payload.length() + "\r\n");
    client.print("\r\n");
    client.print(payload);

    Serial.println("Пакет данных отправлен. Ожидание ответа от сервера...");

    // --- Шаг 3: Надежное ожидание и проверка ответа от сервера ---
    unsigned long timeout = millis();
    String response_header = "";
    String response_body = "";
    bool header_parsed = false;

    // Ждем ответа до 15 секунд
    while (client.connected() && millis() - timeout < 15000L) {
      while (client.available()) {
        char c = client.read();
        // Сначала читаем заголовки, потом тело ответа
        if (!header_parsed) {
          response_header += c;
          // Ищем конец заголовков
          if (response_header.endsWith("\r\n\r\n")) {
            header_parsed = true;
          }
        } else {
          response_body += c;
        }
      }
    }

    // Выводим полный ответ для отладки
    Serial.println("--- Ответ сервера ---");
    Serial.print(response_header);
    Serial.print(response_body);
    Serial.println("-----------------------");

    // Проверяем, содержит ли тело ответа "OK"
    if (response_body.indexOf("OK") != -1) {
      Serial.println("УСПЕХ! Сервер подтвердил получение данных.");
      success = true;
    } else {
      Serial.println("ОШИБКА: Не получен корректный ответ от сервера или ответ пуст.");


      String myPhoneNumber = "+79055191010"; // ЗАМЕНИ НА СВОЙ НОМЕР
      String bootMessage = "ошибка передачи";
      sendSms(myPhoneNumber, bootMessage);
    }

    client.stop();
    Serial.println("Соединение закрыто.");

  } else {
    Serial.println("ОШИБКА: Не удалось подключиться к серверу.");
    success = false;
  }

  modem.gprsDisconnect();
  Serial.println("GPRS отключен.");

  return success;
}


// // --- Функция отправки: Принимает готовый пакет и отправляет его ---
// // Возвращает true в случае успеха, false - в случае любой ошибки.
// bool sendPayloadToServer(const String& payload) {
//   //TODO: здесь надо начать измерение времени сколько ищем сеть
//   if (payload.length() == 0) {
//     Serial.println("Пакет для отправки пуст, отмена.");
//     return false;
//   }

//   SerialGSM.begin(9600, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
//   Serial.println("Инициализация модема...");
//   if (!modem.restart()) {
//     Serial.println("Не удалось перезагрузить модем!");
//     return false;
//   }
//   Serial.println("Модем готов.");

  
//   Serial.print("Ожидание сети...");
//   if (!modem.waitForNetwork()) {
//     Serial.println(" сеть не найдена.");
//     return false;
//   }
//   Serial.println(" сеть найдена.");

//   Serial.print("Подключение к GPRS...");
//   if (!modem.gprsConnect(apn, gprs_user, gprs_pass)) {
//     Serial.println(" не удалось подключиться.");
//     return false;
//   }
//   Serial.println(" GPRS подключен.");

  
//   bool success = false;
//   Serial.print("Отправка данных на сервер ");
//   Serial.print(receiver_server_address);
//   Serial.print(":");
//   Serial.println(receiver_server_port);

//   Serial.println(payload);

//   if (client.connect(receiver_server_address, receiver_server_port)) {
//     //TODO: здесь надо заканчивать измерение времени сколько искали сеть и добавлять к посылке - проблема в том что строки в посылке уже сформированы
//     // поэтому это должен быть какой-то другой формат строки содержащий параметры технические

//     // сколько секунд искалась связь - только что замерили
//     // качество сигнала - узнать как его получать
//     // попытка с которой это сообщение передано - это считаем значение из счетчика ошибок

//     Serial.println("Соединение с сервером установлено.");

//     client.print(String("POST / HTTP/1.1\r\n"));
//     client.print(String("Host: ") + receiver_server_address + "\r\n");
//     client.print("Connection: close\r\n");
//     client.print("Content-Type: text/csv\r\n");
//     client.print(String("Content-Length: ") + payload.length() + "\r\n");
//     client.print("\r\n");
//     client.print(payload);

//     Serial.println("Пакет данных отправлен.");
//     // ВАЖНО: Мы считаем отправку успешной, как только данные ушли в сокет.
//     // Ответ сервера можно игнорировать или просто выводить для отладки.
//     success = true;

//     unsigned long timeout = millis();
//     while (client.connected() && millis() - timeout < 5000L) {
//       while (client.available()) {
//         Serial.write(client.read());
//       }
//     }
//     Serial.println();

//     client.stop();
//     Serial.println("Соединение закрыто.");
//   } else {
//     Serial.println("Не удалось подключиться к серверу.");
//     success = false;
//   }

//   modem.gprsDisconnect();
//   Serial.println("GPRS отключен.");

//   return success;
// }



// Готовит пакет, отправляет его и, в случае успеха, очищает ВЕСЬ лог.
bool attemptToSendLogs(String messageText) {
  Serial.println("\n--- Попытка отправки накопленных логов ---");

  // String payloadToSend = prepareLogPayload();

  // if (payloadToSend.isEmpty()) {
  //   Serial.println("Очередь логов пуста. Нечего отправлять.");
  //   return false;
  // }

  // bool wasSent = sendPayloadToServer(payloadToSend);
  bool wasSent = sendPayloadToServer(messageText);
  
  if (wasSent) {
    Serial.println("УСПЕХ: Самые свежие данные были отправлены. Очищаю весь файл логов.");
    // LittleFS.remove(LOG_FILE_PATH);

    // TODO: обнуляем счетчик ошибок
    return true;
  }

  Serial.println("ОШИБКА: Не удалось отправить данные. Файл логов сохранен для следующей попытки.");

  // TODO: увеличиваем счетчик ошибок
  return false;
}