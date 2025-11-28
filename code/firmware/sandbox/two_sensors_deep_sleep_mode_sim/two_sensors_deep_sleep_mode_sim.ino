#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <MS5837.h>
#include "RTClib.h"
#include "FS.h"
#include "SPIFFS.h"
// #include <SoftwareSerial.h> // <-- УДАЛЕНО: SoftwareSerial не нужен для ESP32

// --- Настройки ---

// Пины для стандартной шины I2C (BME280 и DS3231)
#define I2C0_SDA 21
#define I2C0_SCL 22

// Пины для второй шины I2C (для MS5837)
#define I2C1_SDA 8
#define I2C1_SCL 9

// Настройки глубокого сна
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP  3600

// Имя файла для логов
#define LOG_FILE "/water_level_log.csv"

// Константы для расчета
const float DENSITY_OF_FRESH_WATER = 997.0;
const float GRAVITY_ACCELERATION = 9.80665;

// --- Настройки SIM800L ---
#define SIM800_PWR_PIN 4   // Пин ESP32, подключенный к базе транзистора для управления питанием SIM800L
#define SIM800_RX_PIN  16  // RX на ESP32, подключается к TX на SIM800L
#define SIM800_TX_PIN  17  // TX на ESP32, подключается к RX на SIM800L

// Настройки для отправки данных
#define PHONE_NUMBER "+79123456789" // <-- ВАШ НОМЕР ТЕЛЕФОНА для SMS

// Настройки для отправки на сервер (GPRS)
const char apn[]      = "internet"; // <-- APN вашего оператора (например, "internet.mts.ru", "internet.beeline.ru")
const char server[]   = "your-server.com"; // <-- АДРЕС ВАШЕГО СЕРВЕРА
const char resource[] = "/api/datalogger.php"; // <-- Путь к скрипту на сервере

// --- Объекты ---
Adafruit_BME280 bme;
MS5837 ms5837;
RTC_DS3231 rtc;
// ИЗМЕНЕНО: Используем аппаратный UART2 (Serial2) вместо SoftwareSerial
HardwareSerial simSerial(2);

// --- Прототипы функций ---
void logDataToFile(String data);
bool powerUpSim800l();
void powerDownSim800l();
String sendATCommand(String cmd, long timeout, bool debug);
bool sendSms(String message);
bool sendDataToServer(String data);


void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- Автономный логгер уровня воды v2.0 ---");

  // Убедимся, что SIM800L выключен при старте
  pinMode(SIM800_PWR_PIN, OUTPUT);
  digitalWrite(SIM800_PWR_PIN, LOW);

  // --- Инициализация файловой системы SPIFFS ---
  if (!SPIFFS.begin(true)) {
    Serial.println("Ошибка монтирования SPIFFS!");
    ESP.restart();
  }

  // --- Инициализация BME280 и RTC на первой шине I2C ---
  // Активируем первую шину I2C
  Wire.begin(I2C0_SDA, I2C0_SCL);
  if (!bme.begin(0x76, &Wire)) Serial.println("Не удалось найти датчик BME280!");
  if (!rtc.begin()) Serial.println("Не удалось найти RTC!");
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // --- Инициализация MS5837 на второй шине I2C ---
  // Активируем вторую шину I2C
  Wire.begin(I2C1_SDA, I2C1_SCL);
  if (!ms5837.init()) {
      Serial.println("Ошибка инициализации MS5837!");
  } else {
    ms5837.setModel(MS5837::MS5837_30BA);
    ms5837.setFluidDensity(DENSITY_OF_FRESH_WATER);
  }

  // --- Основная логика ---

  // 1. Получаем текущее время
  // Для работы с RTC нужно убедиться, что активна первая шина I2C
  Wire.begin(I2C0_SDA, I2C0_SCL);
  DateTime now = rtc.now();
  char timestamp[25];
  sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

  // 2. Чтение данных с датчиков
  // Снова переключаемся на первую шину для BME280
  Wire.begin(I2C0_SDA, I2C0_SCL);
  float pressure_atmosphere_mbar = bme.readPressure() / 100.0F;

  // Переключаемся на вторую шину для MS5837
  Wire.begin(I2C1_SDA, I2C1_SCL);
  ms5837.read();
  float pressure_total_mbar = ms5837.pressure();
  float water_temp = ms5837.temperature();

  // 3. Расчеты
  float water_depth_cm = -1.0;
  if (!isnan(pressure_total_mbar) && !isnan(pressure_atmosphere_mbar)) {
    float pressure_water_mbar = pressure_total_mbar - pressure_atmosphere_mbar;
    if (pressure_water_mbar < 0) pressure_water_mbar = 0;
    float pressure_water_pa = pressure_water_mbar * 100.0;
    water_depth_cm = (pressure_water_pa / (DENSITY_OF_FRESH_WATER * GRAVITY_ACCELERATION)) * 100.0;
  } else {
    Serial.println("Ошибка чтения данных с одного из датчиков!");
  }

  // 4. Формирование строки для записи и отправки
  String dataString = String(timestamp) + "," +
                      String(water_depth_cm, 2) + "," +
                      String(water_temp, 2) + "," +
                      String(pressure_atmosphere_mbar, 2) + "," +
                      String(pressure_total_mbar, 2);

  Serial.print("Собранные данные: ");
  Serial.println(dataString);

  // 5. Запись данных в файл
  logDataToFile(dataString);

  // 6. Включение SIM800L и отправка данных
  if (powerUpSim800l()) {
    Serial.println("SIM800L включен и готов к работе.");

    // --- ВЫБЕРИТЕ ОДИН ИЗ СПОСОБОВ ОТПРАВКИ ---
    // Раскомментируйте нужную строку и закомментируйте другую

    // Способ 1: Отправка данных на сервер через GPRS (рекомендуется)
    sendDataToServer(dataString);

    // Способ 2: Отправка данных через SMS
    // sendSms("Water level data: " + dataString);

    // Выключаем модуль для экономии энергии
    powerDownSim800l();
  } else {
    Serial.println("Не удалось запустить SIM800L. Пропускаю отправку данных.");
  }

  // 7. Подготовка и уход в глубокий сон
  Serial.printf("Ухожу в сон на %d секунд...\n", TIME_TO_SLEEP);
  Serial.flush();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {
  // Этот код никогда не будет выполнен
}

// --- Функции для работы с файлами ---
void logDataToFile(String data) {
  // ... (ваш код без изменений)
  Serial.printf("Запись в файл %s\n", LOG_FILE);
  File file = SPIFFS.open(LOG_FILE, FILE_APPEND);
  if (!file) {
    Serial.println("Не удалось открыть файл для записи, создаю новый.");
    file = SPIFFS.open(LOG_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("Не удалось создать файл!");
        return;
    }
  }
  if (file.size() == 0) {
    file.println("Timestamp,Water_Depth_cm,Water_Temp_C,Atm_Pressure_mbar,Total_Pressure_mbar");
  }
  if (file.println(data)) {
    Serial.println("Данные успешно записаны в файл.");
  } else {
    Serial.println("Ошибка записи данных в файл.");
  }
  file.close();
}

// --- Функции для работы с SIM800L ---

// Включение модуля и ожидание регистрации в сети
bool powerUpSim800l() {
  Serial.println("Включаю питание SIM800L...");
  digitalWrite(SIM800_PWR_PIN, HIGH);
  delay(1000); // Даем время на стабилизацию питания

  // ИЗМЕНЕНО: Инициализируем HardwareSerial на нужных пинах
  simSerial.begin(9600, SERIAL_8N1, SIM800_RX_PIN, SIM800_TX_PIN);
  Serial.println("Ожидание ответа от SIM800L...");

  // Ждем, пока модуль начнет отвечать на AT-команды
  int retries = 10;
  while (retries-- > 0) {
    if (sendATCommand("AT\r\n", 1000, true).indexOf("OK") != -1) {
      break;
    }
    delay(1000);
  }
  if (retries <= 0) {
    Serial.println("SIM800L не отвечает.");
    powerDownSim800l();
    return false;
  }

  // Ждем регистрации в сети
  Serial.println("Ожидание регистрации в сети...");
  retries = 20;
  while (retries-- > 0) {
    String response = sendATCommand("AT+CREG?\r\n", 1000, false);
    if (response.indexOf("+CREG: 0,1") != -1 || response.indexOf("+CREG: 0,5") != -1) {
      Serial.println("Регистрация в сети успешна.");
      sendATCommand("ATE0\r\n", 1000, true); // Отключаем эхо
      return true;
    }
    delay(2000);
  }

  Serial.println("Не удалось зарегистрироваться в сети.");
  powerDownSim800l();
  return false;
}

// Выключение питания модуля
void powerDownSim800l() {
  Serial.println("Выключаю питание SIM800L.");
  digitalWrite(SIM800_PWR_PIN, LOW);
}

// Отправка SMS
bool sendSms(String message) {
  Serial.println("Отправка SMS...");
  sendATCommand("AT+CMGF=1\r\n", 2000, true); // Включаем текстовый режим
  String cmd = "AT+CMGS=\"" + String(PHONE_NUMBER) + "\"\r\n";
  if (sendATCommand(cmd, 2000, true).indexOf(">") != -1) {
    simSerial.print(message);
    simSerial.write(26); // Ctrl+Z для отправки
    String response = sendATCommand("", 10000, true); // Ждем ответа об отправке
    if (response.indexOf("OK") != -1) {
      Serial.println("SMS успешно отправлено.");
      return true;
    }
  }
  Serial.println("Ошибка отправки SMS.");
  return false;
}

// Отправка данных на сервер по GPRS
bool sendDataToServer(String data) {
  Serial.println("Отправка данных на сервер...");

  // 1. Настройка GPRS
  if (sendATCommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n", 2000, true).indexOf("OK") == -1) return false;
  if (sendATCommand("AT+SAPBR=3,1,\"APN\",\"" + String(apn) + "\"\r\n", 2000, true).indexOf("OK") == -1) return false;

  // 2. Открытие GPRS соединения
  if (sendATCommand("AT+SAPBR=1,1\r\n", 10000, true).indexOf("OK") == -1) {
    Serial.println("Не удалось открыть GPRS соединение.");
    return false;
  }

  // 3. Инициализация HTTP
  if (sendATCommand("AT+HTTPINIT\r\n", 2000, true).indexOf("OK") == -1) return false;
  if (sendATCommand("AT+HTTPPARA=\"CID\",1\r\n", 2000, true).indexOf("OK") == -1) return false;

  // Формируем URL с данными. Пример: http://your-server.com/api/datalogger.php?data=...
  // Важно! Данные нужно кодировать для URL, но для простоты пока отправляем как есть.
  String url = "AT+HTTPPARA=\"URL\",\"http://" + String(server) + String(resource) + "?data=" + data + "\"\r\n";
  if (sendATCommand(url, 2000, true).indexOf("OK") == -1) return false;

  // 4. Отправка GET запроса
  if (sendATCommand("AT+HTTPACTION=0\r\n", 10000, true).indexOf("OK") == -1) return false;

  // Ждем ответа от сервера, например "+HTTPACTION: 0,200,54" где 200 - код успеха
  String response = sendATCommand("", 10000, true);
  if (response.indexOf(",200,") != -1) {
    Serial.println("Данные успешно отправлены на сервер (Код 200 OK).");
  } else {
    Serial.println("Сервер вернул ошибку или не ответил.");
    Serial.print("Ответ: ");
    Serial.println(response);
  }

  // 5. Закрытие соединений
  sendATCommand("AT+HTTPTERM\r\n", 2000, true);
  sendATCommand("AT+SAPBR=0,1\r\n", 2000, true);

  return true;
}

// Вспомогательная функция для отправки AT-команд и получения ответа
String sendATCommand(String cmd, long timeout, bool debug) {
  String response = "";
  if (cmd.length() > 0) {
    simSerial.print(cmd);
  }
  long startTime = millis();
  while ((millis() - startTime) < timeout) {
    if (simSerial.available()) {
      char c = simSerial.read();
      response += c;
    }
  }
  if (debug) {
    Serial.print("<- ");
    Serial.print(cmd.substring(0, cmd.length() - 2)); // Печатаем без \r\n
    Serial.print("\n-> ");
    Serial.println(response);
  }
  return response;
}
