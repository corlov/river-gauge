#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER   64
#define TINY_GSM_HAS_SMS      0
#define TINY_GSM_HAS_CALLS    0
#define TINY_GSM_HAS_LOCATION 0
#define TINY_GSM_HAS_FTP      0
#define TINY_GSM_HAS_BLUETOOTH 0
#define TINY_GSM_HAS_MQTT     0
#define TINY_GSM_HAS_SSL      0

#include <TinyGsmClient.h>
#include <Wire.h>
#include "RTClib.h"
#include <SparkFunBME280.h>

BME280 bme; 
RTC_DS3231 rtc;

#define DEVICE_ID 7001

const char apn[]    = "internet";
const char server[] = "94.228.123.208";
const int  port     = 8001;

#define SerialGSM Serial1
TinyGsm modem(SerialGSM);
TinyGsmClient client(modem);

#define LED_PIN         17
#define MODEM_POWER_PIN 15


#define DONE_PIN         4

// Пин, к которому подключен датчик (через резистор)
const int SENSOR_PIN = A0;

// Сопротивление вашего резистора в Омах.
const float RESISTOR_VALUE = 250.0;

// Максимальный диапазон измерения вашего датчика в метрах.
const float SENSOR_RANGE_METERS = 5.0;

// Опорное напряжение Arduino. Для Uno/Nano/Mega это обычно 5.0В.
const float V_REF = 5.0;

// Количество замеров для медианного фильтра.
const int NUM_SAMPLES = 15;

// Массив для хранения замеров
int samples[NUM_SAMPLES];


#define VOLTAGE_PIN A1      // Пин для считывания напряжения



// Подключаем необходимые библиотеки
#include <OneWire.h>
#include <DallasTemperature.h>

// Указываем, к какому цифровому пину подключена линия данных датчика
#define WATER_TEMPERATURE_SENSOR_PIN         5

// Создаем объект OneWire для управления шиной
OneWire oneWire(WATER_TEMPERATURE_SENSOR_PIN);

// Передаем ссылку на объект OneWire в библиотеку DallasTemperature
DallasTemperature sensors(&oneWire);





// Номиналы резисторов (используйте .0, чтобы заставить компилятор считать в float)
const float R1 = 17500.0;
const float R2 = 10000.0;

float getActualWaterLevel() {
  for (int i = 0; i < NUM_SAMPLES; i++) {
    samples[i] = analogRead(SENSOR_PIN);
    delay(20);
  }

  for (int i = 0; i < NUM_SAMPLES - 1; i++) {
    for (int j = 0; j < NUM_SAMPLES - i - 1; j++) {
      if (samples[j] > samples[j + 1]) {
        int temp = samples[j];
        samples[j] = samples[j + 1];
        samples[j + 1] = temp;
      }
    }
  }

  // FIXME: я бы сырое значение тоже передавал, хотя его можно получить из данных
  float medianRawValue = samples[8];

  // --- Шаг 4: Преобразуем медианное значение в напряжение ---
  float voltage = (medianRawValue / 1023.0) * V_REF;

  // --- Шаг 5: Преобразуем напряжение в ток (в миллиамперах) ---
  float current_mA = (voltage / RESISTOR_VALUE) * 1000.0;

  // --- Шаг 6: Преобразуем ток в уровень воды (в метрах) ---
  float useful_current = current_mA - 4.0;
  const float current_range = 16.0;
  float waterLevel = (useful_current / current_range) * SENSOR_RANGE_METERS;

  if (waterLevel < 0) {
    waterLevel = 0.0;
  }

  return waterLevel;
}


float getWaterTemperature() {
  sensors.begin();
  delay(1000);
  sensors.requestTemperatures(); // Эта команда не блокирует выполнение

  // Получаем температуру с первого найденного на шине датчика (индекс 0)
  // и выводим ее.
  float tempC = sensors.getTempCByIndex(0);

  // Проверяем, не вернул ли датчик ошибку
  // (значение -127 означает, что датчик не найден или неисправен)
  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("E4");
    delay(1000); // Ждем секунду перед новой попыткой
    return;
  }

  return tempC;
}


/**
 * @brief Включает делитель, измеряет напряжение и выключает делитель.
 * @return Напряжение аккумулятора в Вольтах.
 */
float readBatteryVoltage() {  
  // 3. Считываем значение с АЦП (0-1023)
  int adcValue = analogRead(VOLTAGE_PIN);

  // 5. Конвертируем значение АЦП обратно в напряжение
  // Сначала находим напряжение на пине A0
  float dividerVoltage = adcValue * (5.0 / 1023.0);

  // Затем, зная напряжение на делителе, вычисляем исходное напряжение аккумулятора
  // Формула: V_in = V_out * (R1 + R2) / R2
  float batteryVoltage = dividerVoltage * (R1 + R2) / R2;

  return batteryVoltage;
}


void setup() {
  // TODO: проверить - если до этого была неуспешная попытка отправки, то пробуем в любом случае отправить
  // если была успешная то отправим когда придет время отрпавки

  pinMode(LED_PIN, OUTPUT);
  pinMode(MODEM_POWER_PIN, OUTPUT);

  pinMode(DONE_PIN, OUTPUT);
  digitalWrite(DONE_PIN, LOW);

  digitalWrite(MODEM_POWER_PIN, LOW);

  Serial.begin(9600);
  delay(3000); 

  digitalWrite(MODEM_POWER_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);

  // На Pro Micro это пины 2(SDA) и 3(SCL)
  Wire.begin();
  
  if (!rtc.begin()) {
    Serial.println(F("E1"));
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  bme.setI2CAddress(0x76);
  if (!bme.beginI2C()) { 
    Serial.println(F("E2")); 
  }



  digitalWrite(MODEM_POWER_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  initNetwork();  

  char postData[50];
  formatPostData(postData, sizeof(postData));
  //Serial.println(postData);

  if (client.connect(server, port)) {
    client.print(postData);    
    client.stop();
  } 
  else {
    //Serial.println(F("E3"));
  }

  modem.gprsDisconnect();
  
  digitalWrite(MODEM_POWER_PIN, LOW);
  //digitalWrite(LED_PIN, HIGH);
  
  // TODO: отправить сигнал DONE чтобы обесточить цепь
  // сохранить результаты во флеш-памяти, если не смогли отправить и счетчик неудачных попыток увеличить
  // при этом данные о температуре общие можно всегда скидывать в память ЕЕЕПРОМ чтобы больше информации с устройства иметь
  digitalWrite(DONE_PIN, HIGH);
}



void initNetwork() {
  SerialGSM.begin(9600);
  Serial.println(F("Initializing modem..."));
  if (!modem.restart()) {
    Serial.println(F("Failed"));
    while (true); 
  }
  Serial.println(F("Ready"));
  
  if (!modem.waitForNetwork()) {
    Serial.println(F("fail."));
    while (true);
  }
  Serial.println(F("NET OK"));

  if (!modem.gprsConnect(apn)) {
    Serial.println(F("fail."));
    while (true);
  }  
  Serial.println(F("GPRS OK"));
}


void formatPostData(char* buffer, size_t bufferSize) {
  // 1. Считываем данные
  DateTime now = rtc.now();
  float temperature = bme.readTempC();
  float humidity = bme.readFloatHumidity();
  float pressure = bme.readFloatPressure() / 100.0F;

  // 2. Преобразуем float в целые числа (long), умножив на 10,
  // чтобы сохранить один знак после запятой.
  long temp_int = temperature * 10;
  long hum_int = humidity * 10;
  long press_int = pressure * 10;

  float waterLevel = getActualWaterLevel();

  float waterTemperature = getWaterTemperature();

  float batteryVoltage = readBatteryVoltage();

  // 3. Формируем строку за один вызов, используя целочисленные типы.
  // snprintf_P читает строку формата (PSTR) из Flash-памяти, экономя SRAM.
  snprintf_P(buffer, bufferSize,
             PSTR("%04d,%04d-%02d-%02d %02d:%02d,%ld.%1ld,%ld.%1ld,%ld.%1ld,%ld.%1ld,%ld.%1ld,%ld.%1ld\n"),
             DEVICE_ID,
             now.year(), now.month(), now.day(), now.hour(), now.minute(),
             temp_int / 10, abs(temp_int % 10),   // Целая и дробная часть температуры
             hum_int / 10, abs(hum_int % 10),     // Целая и дробная часть влажности
             press_int / 10, abs(press_int % 10),  // Целая и дробная часть давления
             waterLevel,
             waterTemperature,
             batteryVoltage
  );
}


void loop() {
  
}


