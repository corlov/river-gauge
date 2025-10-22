#include <Wire.h>
#include "RTClib.h"

// Создаем объект для работы с часами DS3231
RTC_DS3231 rtc;

// --- НАСТРОЙКИ ---
const int ledPin = LED_BUILTIN;
#define SLEEP_DURATION_SECONDS 30 // Время сна в секундах

// Функция для красивой печати времени
void printTime(const DateTime& dt) {
  char buffer[] = "YYYY/MM/DD hh:mm:ss";
  Serial.println(dt.toString(buffer));
}

// Функция для мигания светодиодом
void blinkLED() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
    delay(200);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Небольшая задержка для стабилизации Serial

  pinMode(ledPin, OUTPUT);

  // Инициализация I2C и RTC
  if (!rtc.begin()) {
    Serial.println("Error!");
    while (1) delay(10);
  }

  // =========================================================================
  // ВАЖНО! РАСКОММЕНТИРУЙТЕ ЭТУ СТРОЧКУ ТОЛЬКО ОДИН РАЗ ДЛЯ ПЕРВОЙ ЗАГРУЗКИ,
  // ЧТОБЫ УСТАНОВИТЬ ВРЕМЯ НА ЧАСАХ. ПОСЛЕ ЭТОГО ЗАКОММЕНТИРУЙТЕ ЕЕ СНОВА
  // И ПРОШЕЙТЕ ПЛАТУ ЕЩЕ РАЗ.
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // =========================================================================

  // Проверяем, не потерял ли модуль RTC питание (например, если села батарейка)
  if (rtc.lostPower()) {
    Serial.println("RTC power out setup time before!");
    // Здесь можно добавить логику для повторной установки времени
  }

  Serial.println("Up!");

  // 1. Получаем и печатаем текущее время
  DateTime now = rtc.now();
  Serial.print("Current Time: ");
  printTime(now);

  // 2. Мигаем светодиодом
  blinkLED();

  // 3. Уходим в глубокий сон
  Serial.print("Go to sleep");
  Serial.flush(); // Убедимся, что все данные отправлены в порт перед сном

  esp_sleep_enable_timer_wakeup(SLEEP_DURATION_SECONDS * 1000000); // Время в микросекундах
  esp_deep_sleep_start();
}

void loop() {
  // Этот код никогда не будет выполнен, так как ESP32
  // уходит в сон в конце функции setup() и после пробуждения
  // начинает выполнение снова с setup().
}
