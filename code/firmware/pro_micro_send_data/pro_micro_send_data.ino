/*
#define TINY_GSM_MODEM_SIM800      // Указываем наш модем. Эта строка у тебя уже есть, убедись, что она здесь.
#define TINY_GSM_RX_BUFFER   64    // Уменьшаем буфер для экономии RAM, может немного помочь и с Flash

// Отключаем все ненужные функции для экономии Flash-памяти
// Компилятор просто не будет включать код для этих функций в прошивку
#define TINY_GSM_HAS_SMS      0
#define TINY_GSM_HAS_CALLS    0
#define TINY_GSM_HAS_LOCATION 0
#define TINY_GSM_HAS_FTP      0
#define TINY_GSM_HAS_BLUETOOTH 0
#define TINY_GSM_HAS_MQTT     0
#define TINY_GSM_HAS_SSL      0 // SSL самый "тяжелый", его отключение даст больше всего экономии

*/


// --- НАСТРОЙКИ GPRS И СЕРВЕРА ---
const char apn[]      = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";
const char server[]   = "94.228.123.208";
const int  port       = 8001;

#define LOOP_DELAY_SECONDS 15 // <<< ИЗМЕНЕНО: Задержка в конце цикла вместо сна

// --- ПИНЫ ДЛЯ PRO MICRO ---
#define LED_PIN       17  // <<< ИЗМЕНЕНО: Используем RX LED на пине 17
#define MODEM_RST_PIN 5

// --- БИБЛИОТЕКИ (те же самые) ---
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#include <Wire.h>
#include "RTClib.h"
//#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>




// --- ОБЪЕКТЫ И ПЕРЕМЕННЫЕ ---
RTC_DS3231 rtc;
Adafruit_BME280 bme;

// <<< ИЗМЕНЕНО: Используем аппаратный Serial1 на пинах 0(RX) и 1(TX)
#define SerialGSM Serial1
TinyGsm modem(SerialGSM);
TinyGsmClient client(modem);


void blink() {
  int i = 0;
  while(i++ < 3) {
    digitalWrite(17, HIGH);
    delay(700); // Уменьшим задержку, чтобы быстрее увидеть результат
    digitalWrite(17, LOW);
    delay(200);
  }
}


// <<< ИЗМЕНЕНО: Вся логика перенесена из setup() в loop()
// setup() теперь только для однократной инициализации
void setup() {
  // Запускаем Serial для отладки в мониторе порта
  Serial.begin(9600);
  delay(3000); // Даем время на открытие монитора порта
  //Serial.println(F("--- Pro Micro Meteo Station ---"));

  pinMode(LED_PIN, OUTPUT);

  blink();

  // --- 1. Инициализация датчиков ---
  Wire.begin(); // На Pro Micro это пины 2(SDA) и 3(SCL)
  
  if (!rtc.begin()) {
    Serial.println(F("Error: 1"));
  }

  if (!bme.begin(0x76)) {
    Serial.println(F("Error: 2"));    
  }

  // Раскомментируйте эту строку ОДИН РАЗ для установки времени на RTC
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

/*
  // --- 2. Инициализация модема ---
  pinMode(MODEM_RST_PIN, OUTPUT);
  digitalWrite(MODEM_RST_PIN, HIGH); // Убедимся, что модем не в ресете
  SerialGSM.begin(9600);

  Serial.println(F("Initializing modem..."));
  if (!modem.restart()) {
    Serial.println(F("Failed to restart modem!"));
  }
  Serial.println(F("Modem is ready."));
*/
}

void loop() {
  digitalWrite(LED_PIN, HIGH); // Включаем светодиод, пока работаем


  // // --- 1. Считываем данные с датчиков ---
  // DateTime now = rtc.now();
  // float temperature = bme.readTemperature();
  // float humidity = bme.readHumidity();
  // float pressure = bme.readPressure() / 100.0F;

  // char postData[128];
  // char tempStr[8];
  // char humStr[8];
  // char pressStr[8];

  // // Преобразуем каждое float-число в строку
  // dtostrf(temperature, 4, 1, tempStr); // 4 - общая ширина, 1 - знак после запятой
  // dtostrf(humidity, 4, 1, humStr);
  // dtostrf(pressure, 6, 1, pressStr);   // Давление может быть > 1000, берем ширину побольше

  // snprintf(postData, sizeof(postData), "%04d-%02d-%02d %02d:%02d,%s,%s,%s", now.year(), now.month(), now.day(), now.hour(), now.minute(), tempStr, humStr, pressStr);

  // --- 2. Подключение к сети и отправка ---
  Serial.print(F("Waiting for network..."));
  if (!modem.waitForNetwork()) {
    Serial.println(F(" fail."));
    goToSleep(); // Переходим к задержке, если нет сети
    return;
  }
  Serial.println(F(" OK."));

  Serial.print(F("Connecting to GPRS..."));
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(F(" fail."));
    goToSleep(); // Переходим к задержке
    return;
  }
  Serial.println(F(" OK."));

  Serial.print(F("Connecting to server..."));
  if (client.connect(server, port)) {
    Serial.println(F(" OK."));

    // <<< ИЗМЕНЕНО: Отправляем HTTP POST-запрос без класса String
    client.print(F("POST / HTTP/1.1\r\n"));
    client.print(F("Host: 94.228.123.208\r\n"));
    // client.print(F("Connection: close\r\n"));
    // client.print(F("Content-Type: text/plain\r\n"));
    // client.print(F("Content-Length: "));
    // client.print(strlen(postData));
    client.print(F("123\r\n\r\n"));
    // client.print(postData);

    Serial.println(F("Data sent. Waiting for response..."));

    // Ждем ответа от сервера
    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 5000L) {
      while (client.available()) {
        char c = client.read();
        Serial.write(c);
        timeout = millis();
      }
    }
    Serial.println();
    client.stop();
    Serial.println(F("Connection closed."));

  } else {
    Serial.println(F(" failed."));
  }

  modem.gprsDisconnect();
  //Serial.println(F("GPRS disconnected."));

  // --- 3. Уходим в "сон" ---
  goToSleep();
}

// <<< ИЗМЕНЕНО: Функция сна заменена на простую задержку
void goToSleep() {
  Serial.print(F("Sleeping for "));
  Serial.print(LOOP_DELAY_SECONDS);
  digitalWrite(LED_PIN, LOW); // Выключаем светодиод на время "сна"
  delay(LOOP_DELAY_SECONDS * 1000);
}
