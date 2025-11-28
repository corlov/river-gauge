// --- ПОДКЛЮЧАЕМ ВСЕ НЕОБХОДИМЫЕ БИБЛИОТЕКИ ---
#include <Wire.h>
#include "RTClib.h"             // Для часов DS3231
#include <Adafruit_Sensor.h>    // Зависимость для BME280
#include <Adafruit_BME280.h>    // Для датчика BME280

// --- НАСТРОЙКИ ---
#define LED_PIN LED_BUILTIN         // Встроенный светодиод. На многих ESP32 это GPIO 2
#define SLEEP_DURATION_SECONDS 20   // Время сна в секундах

// --- СОЗДАЕМ ОБЪЕКТЫ ДЛЯ РАБОТЫ С ДАТЧИКАМИ ---
RTC_DS3231 rtc;
Adafruit_BME280 bme;

// Функция для мигания светодиодом, чтобы показать, что мы проснулись
void indicateWakeUp() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(300);
  digitalWrite(LED_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Даем время для стабилизации Serial

  // 1. Индикация пробуждения
  indicateWakeUp();
  Serial.println("\nWaked up!");

  // 2. Инициализация датчиков
  // Запускаем I2C
  Wire.begin();

  // Инициализация RTC (часы)
  if (!rtc.begin()) {
    Serial.println("not found RTC DS3231!");
    while (1) delay(10);
  }

  // Инициализация BME280 (погода)
  if (!bme.begin(0x76)) {
    Serial.println("not found BME280!");
    while (1) delay(10);
  }

  // =========================================================================
  // ВАЖНО! РАСКОММЕНТИРУЙТЕ ЭТУ СТРОЧКУ ТОЛЬКО ОДИН РАЗ ДЛЯ ПЕРВОЙ ЗАГРУЗКИ,
  // ЧТОБЫ УСТАНОВИТЬ ВРЕМЯ НА ЧАСАХ. ПОСЛЕ ЭТОГО ЗАКОММЕНТИРУЙТЕ ЕЕ СНОВА
  // И ПРОШЕЙТЕ ПЛАТУ ЕЩЕ РАЗ.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // =========================================================================

  if (rtc.lostPower()) {
    Serial.println("setup time again!");
  }

  // 3. Считываем и выводим все данные
  

  // Получаем время
  DateTime now = rtc.now();
  char buffer[] = "YYYY/MM/DD hh:mm:ss";
  Serial.print("Timestamp: ");
  Serial.println(now.toString(buffer));

  // Получаем показания погоды
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  Serial.print("  temperature: ");
  Serial.print(temperature);
  Serial.println(" *C");

  Serial.print("  humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("  pressure: ");
  Serial.print(pressure);
  Serial.println(" hPa");
  Serial.println("-----------------------");

  // 4. Уходим в глубокий сон
  Serial.print("Go to sleep");
  Serial.print(SLEEP_DURATION_SECONDS);
  Serial.println(" секунд...");
  Serial.flush(); // Отправляем все из буфера Serial перед сном

  esp_sleep_enable_timer_wakeup(SLEEP_DURATION_SECONDS * 1000000); // Время в микросекундах
  esp_deep_sleep_start();
}

void loop() {
  // Этот код никогда не будет выполнен, так как ESP32
  // перезагружается после глубокого сна и начинает снова с setup().
}
