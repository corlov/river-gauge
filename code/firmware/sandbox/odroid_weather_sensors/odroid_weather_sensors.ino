#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SI1145.h>

// Создаем объекты для каждого датчика
Adafruit_BME280 bme; // I2C
Adafruit_SI1145 uv = Adafruit_SI1145();

void setup() {
  Serial.begin(115200);
  while (!Serial); // Ожидание подключения к консоли

  Serial.println("start test  Weather Board 2...");

  // Инициализируем датчик Si1132 (УФ-индекс, видимый свет)
  if (!uv.begin()) {
    Serial.println("Error: #1 Si1132/SI1145");
    while (1);
  }
  Serial.println("Found Si1132");

  // Инициализируем датчик BME280 (температура, влажность, давление)
  // Адрес 0x76 является стандартным для этой платы
  if (!bme.begin(0x76)) {
    Serial.println("errror 2");
    while (1);
  }
  Serial.println("found BME280");
  Serial.println("---------------------------------------------");
}

void loop() {
  // --- Считываем данные с BME280 ---
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F; // Переводим из Паскалей в гектопаскали (гПа)

  // --- Считываем данные с Si1132 ---
  float visible = uv.readVisible(); // Видимый свет
  float ir = uv.readIR();           // Инфракрасное излучение
  float uv_index = uv.readUV() / 100.0; // УФ-индекс (значение нужно поделить на 100)

  
  

  Serial.print("temp: ");
  Serial.print(temperature);
  Serial.println(" *C");

  Serial.print("hum: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("pressuer: ");
  Serial.print(pressure);
  Serial.println(" hPa");

  // Serial.print("Видимый свет: ");
  // Serial.println(visible);

  // Serial.print("ИК-излучение: ");
  // Serial.println(ir);

  // Serial.print("УФ-индекс: ");
  // Serial.println(uv_index);

  Serial.println("---------------------------------------------");

  // Ждем 2 секунды перед следующим опросом
  delay(2000);
}
