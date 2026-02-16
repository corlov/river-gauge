#include <HardwareSerial.h>

#define MODEM_RX_PIN 16  // ESP32 RX -> SIM800 TX
#define MODEM_TX_PIN 15  // ESP32 TX -> SIM800 RX

HardwareSerial SerialGSM(1);

void setup() {
  Serial.begin(115200);
  SerialGSM.begin(9600, SERIAL_8N1, 16, 15);

  delay(5000); // Ждем запуск модема

  checkNetworkStatus();

  // 2. Ждем регистрации в сети
  waitForNetwork();

  // 3. Проверяем сигнал
  SerialGSM.println("AT+CSQ");
  delay(1000);
  Serial.println("Уровень сигнала: " + readResponse());

  // 4. Отправляем тестовое SMS
  sendSMS("+79055191010", "Тестовое сообщение от ESP32 с SIM800L");

  Serial.println("=== Тест завершен ===");
}

void checkNetworkStatus() {
  Serial.println("\n=== Диагностика сети ===");

  // 1. Проверяем связь с модулем
  SerialGSM.println("AT");
  Serial.println("AT ответ: " + readResponse());

  // 2. Проверяем SIM-карту
  SerialGSM.println("AT+CPIN?");
  Serial.println("SIM статус: " + readResponse());

  // 3. Проверяем оператора
  SerialGSM.println("AT+COPS?");
  Serial.println("Оператор: " + readResponse());

  // 4. Детальная проверка регистрации
  SerialGSM.println("AT+CREG?");
  String resp = readResponse();
  Serial.println("Регистрация: " + resp);

  // 5. Уровень сигнала
  SerialGSM.println("AT+CSQ");
  Serial.println("Сигнал: " + readResponse());
}

String readResponse() {
  String response = "";
  unsigned long startTime = millis();
  while (millis() - startTime < 2000) {
    while (SerialGSM.available()) {
      char c = SerialGSM.read();
      response += c;
    }
  }
  return response;
}

void waitForNetwork() {
  Serial.println("Ожидание регистрации в сети...");

  int attempts = 0;
  while (attempts < 30) { // 30 попыток по 2 секунды = 60 секунд
    SerialGSM.println("AT+CREG?");
    delay(2000);

    String response = readResponse();
    Serial.print("Попытка ");
    Serial.print(attempts + 1);
    Serial.print(": ");
    Serial.println(response);

    // Проверяем статус регистрации
    if (response.indexOf("+CREG: 0,1") >= 0 || response.indexOf("+CREG: 0,5") >= 0) {
      Serial.println("Успешно зарегистрированы в сети!");
      return;
    }

    attempts++;
    delay(1000);
  }

  Serial.println("Не удалось зарегистрироваться в сети!");
  Serial.println("Проверьте уровень сигнала и настройки оператора.");
}


void sendSMS(String number, String message) {
  Serial.println("Отправка SMS...");

  // Устанавливаем текстовый режим SMS
  SerialGSM.println("AT+CMGF=1");
  delay(1000);

  // Указываем номер
  SerialGSM.print("AT+CMGS=\"");
  SerialGSM.print(number);
  SerialGSM.println("\"");
  delay(1000);

  // Пишем текст сообщения
  SerialGSM.print(message);
  delay(1000);

  // Отправляем Ctrl+Z (ASCII 26) для отправки
  SerialGSM.write(26);
  delay(5000); // Ждем отправки

  Serial.println("SMS отправлена (проверьте ответ модема)");

  // Читаем ответ
  String response = readResponse();
  Serial.println("Ответ модема: " + response);
}

void sendCommand(String cmd) {
  Serial.println(">> " + cmd);
  SerialGSM.println(cmd);
  delay(1000);
  Serial.print("<< ");
  while (SerialGSM.available()) {
    Serial.write(SerialGSM.read());
  }
  Serial.println();
}


void loop() {
  
}




// void setup() {
//   Serial.begin(115200);
//   SerialGSM.begin(9600, SERIAL_8N1, 16, 15);

//   delay(5000); // Даем модему время на запуск

//   Serial.println("=== ТЕСТ МОДЕМА SIM800 ===");

//   // Базовый тест
//   sendCommand("AT");
//   sendCommand("AT+CREG?");
//   sendCommand("AT+CSQ");
//   sendCommand("AT+COPS?");
//   sendCommand("AT+CPIN?");
//   sendCommand("AT+CGMM"); // Модель модема
//   sendCommand("AT+CGMR"); // Версия прошивки

//   // Настройка SMS (если нужно)
//   sendCommand("AT+CMGF=1"); // Текстовый режим SMS
// }