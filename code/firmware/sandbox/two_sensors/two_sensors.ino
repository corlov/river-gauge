#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <MS5837.h>

// --- Настройки ---

// Пины для стандартной шины I2C (для BME280)
#define I2C0_SDA 21
#define I2C0_SCL 22

// Пины для второй шины I2C (для MS5837)
#define I2C1_SDA 8
#define I2C1_SCL 9

// Константы для расчета
const float DENSITY_OF_FRESH_WATER = 997.0; // Плотность пресной воды в кг/м^3 при 25°C
const float GRAVITY_ACCELERATION = 9.80665; // Ускорение свободного падения в м/с^2

// --- Объекты датчиков ---
Adafruit_BME280 bme; // Датчик атмосферного давления
MS5837 ms5837;       // Датчик давления воды

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Ожидание подключения к Serial порту
  }
  Serial.println("\n--- Измеритель уровня воды (исправленная версия) ---");

  // --- Инициализация BME280 ---
  Serial.println("Инициализация BME280...");
  // Настраиваем шину Wire на пины для BME280
  Wire.begin(I2C0_SDA, I2C0_SCL);
  // Адрес по умолчанию 0x76. Если у вас 0x77, используйте bme.begin(0x77)
  if (!bme.begin(0x76, &Wire)) {
    Serial.println("Не удалось найти датчик BME280! Проверьте подключение и адрес.");
    while (1) delay(10);
  }
  Serial.println("BME280 найден!");

  // --- Инициализация MS5837 ---
  Serial.println("Инициализация MS5837...");
  // ПЕРЕКЛЮЧАЕМ шину Wire на пины для MS5837
  Wire.begin(I2C1_SDA, I2C1_SCL);
  // Теперь вызываем init() без аргументов. Библиотека будет использовать глобальный Wire,
  // который мы только что настроили на нужные пины.
  if (!ms5837.init()) {
      Serial.println("Ошибка инициализации MS5837! Проверьте подключение.");
      while (1) delay(10);
  }

  // Устанавливаем модель датчика. MS5837-30BA - самая распространенная.
  ms5837.setModel(MS5837::MS5837_30BA);
  ms5837.setFluidDensity(DENSITY_OF_FRESH_WATER);

  Serial.println("MS5837 готов к работе!");
  Serial.println("---------------------------------");
}

void loop() {
  // --- Чтение данных с BME280 ---
  // 1. Переключаем шину I2C на пины для BME280
  Wire.begin(I2C0_SDA, I2C0_SCL);
  // 2. Читаем данные
  // Давление из библиотеки Adafruit BME280 приходит в Паскалях, переводим в мбар
  float pressure_atmosphere_mbar = bme.readPressure() / 100.0F;

  // --- Чтение данных с MS5837 ---
  // 1. Переключаем шину I2C на пины для MS5837
  Wire.begin(I2C1_SDA, I2C1_SCL);
  // 2. Читаем данные
  ms5837.read();
  float pressure_total_mbar = ms5837.pressure(); // Давление в миллибарах
  float water_temp = ms5837.temperature();

  // Проверка на валидность данных
  if (isnan(pressure_total_mbar) || isnan(pressure_atmosphere_mbar)) {
    Serial.println("Ошибка чтения данных с одного из датчиков!");
    delay(1000);
    return;
  }

  // --- Расчеты ---
  float pressure_water_mbar = pressure_total_mbar - pressure_atmosphere_mbar;
  if (pressure_water_mbar < 0) {
    pressure_water_mbar = 0;
  }
  float pressure_water_pa = pressure_water_mbar * 100.0;
  float water_depth_cm = (pressure_water_pa / (DENSITY_OF_FRESH_WATER * GRAVITY_ACCELERATION)) * 100.0;

  // --- Вывод в Serial порт ---
  Serial.print("Атм. давление (BME280): ");
  Serial.print(pressure_atmosphere_mbar);
  Serial.println(" мбар");

  Serial.print("Общее давление (MS5837): ");
  Serial.print(pressure_total_mbar);
  Serial.println(" мбар");

  Serial.print("Давление воды: ");
  Serial.print(pressure_water_mbar);
  Serial.println(" мбар");

  Serial.print("Температура воды: ");
  Serial.print(water_temp);
  Serial.println(" °C");

  Serial.println("=================================");
  Serial.print(">>> Уровень воды: ");
  Serial.print(water_depth_cm);
  Serial.println(" см");
  Serial.println("=================================\n");

  delay(2000);
}
