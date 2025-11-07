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

  // Инициализируем шину I2C. Для ESP32-S3 можно не указывать пины,
  // если используются стандартные (SDA=21, SCL=22)
  Wire.begin();

  // Инициализируем датчик BH1750
  // По умолчанию используется режим непрерывного измерения с высоким разрешением
  if (lightMeter.begin()) {
    Serial.println(F("init ok"));
  } else {
    Serial.println(F("Error"));
    // Бесконечный цикл, если датчик не найден
    while (1) {}
  }

  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Считываем уровень освещенности в люксах (лк)
  float lux = lightMeter.readLightLevel();

  // Выводим показания в монитор порта (консоль)
  Serial.print("Lightness: ");
  Serial.print(lux);
  Serial.println(" lux");

  // Ждем 1 секунду перед следующим измерением
  delay(1000);

  digitalWrite(ledPin, HIGH);   // Включаем светодиод (HIGH - высокий уровень напряжения)
  Serial.println("LED ON");
  delay(200);                  // Ждем одну секунду (1000 миллисекунд)
  digitalWrite(ledPin, LOW);    // Выключаем светодиод (LOW - низкий уровень напряжения)
  Serial.println("LED OFF");
  delay(200);  
}





