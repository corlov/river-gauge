// Подключаем библиотеку, которую только что установили
#include <Adafruit_NeoPixel.h>

// --- Настройки для нашего встроенного светодиода ---

// Указываем РЕАЛЬНЫЙ пин, к которому подключен светодиод.
// Для большинства плат ESP32-S3 это GPIO 48.
#define NEOPIXEL_PIN 48

// У нас всего один светодиод на плате.
#define NUMPIXELS 1

// Создаем объект "pixels", через который будем управлять светодиодом.
// Ему нужно знать, сколько у нас светодиодов, на каком они пине, и их тип.
// Для ESP32-S3 эти параметры стандартные.
Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // Инициализируем библиотеку. Это нужно сделать один раз.
  pixels.begin();

  // Установим яркость, чтобы не слепил глаза (0-255)
  pixels.setBrightness(50);
}

void loop() {
  pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.show();
  delay(1000); // Ждем 1 секунду

  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();
  delay(1000); // Ждем 1 секунду

  
}
