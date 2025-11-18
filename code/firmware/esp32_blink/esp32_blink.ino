// Подключаем необходимые библиотеки
#include <Wire.h>      // Библиотека для работы с I2C
#include <BH1750.h>    // Библиотека для датчика BH1750

const int ledPin = LED_BUILTIN;

// Создаем объект для работы с датчиком
BH1750 lightMeter;

void setup() {
  // Инициализируем последовательный порт (консоль) на скорости 115200 бод
  // Для ESP32 рекомендуется использовать скорость выше, чем 9600
  Serial.begin(115200);



  pinMode(ledPin, OUTPUT);
}

void loop() {
  digitalWrite(ledPin, HIGH);   // Включаем светодиод (HIGH - высокий уровень напряжения)
  Serial.println("LED ON");
  delay(2000);                  // Ждем одну секунду (1000 миллисекунд)
  digitalWrite(ledPin, LOW);    // Выключаем светодиод (LOW - низкий уровень напряжения)
  Serial.println("LED OFF");
  delay(12000);  

  Serial.println("--- Pin Definitions ---");
  Serial.print("A0 is GPIO: ");
  Serial.println(A0);

  Serial.print("A1 is GPIO: ");
  Serial.println(A1);

  Serial.print("LED_BUILTIN is GPIO: ");
  Serial.println(LED_BUILTIN);
  Serial.println("---------------------");


  Serial.println("\n--- Карта пинов для вашей платы ESP32-S3 ---");

  // --- Аналоговые пины (псевдонимы для GPIO) ---
  Serial.println("\n--- Аналоговые пины (ADC1) ---");
  Serial.print("A0 = GPIO "); Serial.println(A0);
  Serial.print("A1 = GPIO "); Serial.println(A1);
  Serial.print("A2 = GPIO "); Serial.println(A2);
  Serial.print("A3 = GPIO "); Serial.println(A3);
  Serial.print("A4 = GPIO "); Serial.println(A4);
  // На ESP32-S3 аналоговых пинов гораздо больше,
  // но это самые стандартные "Arduino-совместимые" имена.

  // --- Встроенные устройства ---
  Serial.println("\n--- Встроенные устройства ---");
  Serial.print("LED_BUILTIN = "); Serial.println(LED_BUILTIN);
  if (LED_BUILTIN > 60) {
    Serial.println(" (Это 'виртуальный' пин для RGB светодиода)");
  }

  // --- Стандартные интерфейсы ---
  Serial.println("\n--- Аппаратный I2C (Wire) ---");
  Serial.print("SDA = GPIO "); Serial.println(SDA);
  Serial.print("SCL = GPIO "); Serial.println(SCL);

  Serial.println("\n--- Аппаратный SPI ---");
  Serial.print("MOSI = GPIO "); Serial.println(MOSI);
  Serial.print("MISO = GPIO "); Serial.println(MISO);
  Serial.print("SCK = GPIO "); Serial.println(SCK);
  Serial.print("SS (CS) = GPIO "); Serial.println(SS);

  // --- Стандартные пины для Serial (UART0) ---
  Serial.println("\n--- Аппаратный Serial (UART0) ---");
  Serial.print("TX = GPIO "); Serial.println(TX);
  Serial.print("RX = GPIO "); Serial.println(RX);

  Serial.println("\n------------------------------------------");
}






