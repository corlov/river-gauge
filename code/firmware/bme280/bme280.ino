// Подключаем необходимые библиотеки
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


const int ledPin = LED_BUILTIN;

Adafruit_BME280 bme;

void setup() {  
  Serial.begin(115200);

  // Инициализируем шину I2C. Для ESP32-S3 можно не указывать пины,
  // если используются стандартные (SDA=21, SCL=22)
  Wire.begin();

  if (!bme.begin(0x76)) {
    Serial.println("Error: sensor not found BME280");
    while (1) delay(10);
  }

  pinMode(ledPin, OUTPUT);
}

void loop() {  
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  Serial.print("temperature: ");
  Serial.print(temperature);
  Serial.println(" *C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println(" hPa");
  Serial.println("---------------------------------------------");

  delay(1000);
  digitalWrite(ledPin, HIGH);   // Включаем светодиод (HIGH - высокий уровень напряжения)
  delay(100);                  // Ждем одну секунду (1000 миллисекунд)
  digitalWrite(ledPin, LOW);    // Выключаем светодиод (LOW - низкий уровень напряжения)  
  delay(100); 
}
