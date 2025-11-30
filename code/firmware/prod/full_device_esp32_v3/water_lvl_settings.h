// =============================================================================
// *********************************************
// общие настройки устройства
// *********************************************
#define VERSION "1.0.0"
#define INSTALL_DATE "01.12.2025"
#define DEVICE_ID 7001
#define GPS_LON 56.065226 
#define GPS_LAT 36.234294
const char apn[] = "internet";
const char gprs_user[] = "";
const char gprs_pass[] = "";
const char receiver_server_address[] = "89.169.3.241";
const int receiver_server_port = 8001;

// *********************************************
// аппаратные настройки пинов
// *********************************************
#define NEOPIXEL_PIN                    48

#define LED_PIN LED_BUILTIN

#define MODEM_POWER_PIN                 12
#define MODEM_RX_PIN                    15
#define MODEM_TX_PIN                    16
#define DONE_PIN                        4

#define WATER_LEVEL_SENSOR_PIN          14
#define BATTERY_VOLTAGE_PIN             5

#define WATER_TEMPERATURE_SENSOR_PIN    5
//#define RESET_MODEM_PIN 14

#define PRO_MICRO_ADDRESS 8 // Адрес нашего "Специалиста"



// FIXME: скорректировать
const int unusedPins[] = {0, 1, 2, 3, /*4, 5,*/ 6, 7, 8, 9, 10, 11,12,13,14,15,
                          16,17,18,19,20,21, 35,40,41,42,/*43,44,*/45,
                          46,47/*,48*/};

// *********************************************
// настройки для датчика измерения уровня воды
// *********************************************
// сопротивление шунтирующего резистора в Омах около датчика уровня воды
const float WATER_LEVEL_SHUNT_RESISTOR_VALUE = 150.0; // максимум 165 Ом, но не более!!!
// максимальный диапазон измерения датчика глубины в метрах 
// FIXME: разве не 10 метров у него?
const float WATER_LEVEL_SENSOR_RANGE_METERS = 5.0;
// Опорное напряжение Arduino. Для Uno/Nano/Mega это обычно 5.0В.
// FIXME: возможно это д.б. 3.3В для ESP?
const float V_REF = 3.3;
// Количество замеров для медианного фильтра.
const int SAMPLES_SIZE = 15;
// Массив для хранения замеров
extern int wl_measure_samples[SAMPLES_SIZE];

// *********************************************
// Номиналы резисторов делителя напряжения
// *********************************************
const float VOLTAGE_DIVIDER_UP_RESISTOR = 40000.0; //33000 был но входное может быть больше лучше перестраховаться
const float VOLTAGE_DIVIDER_DOWN_RESISTOR = 12000.0; //10000.0;
// =============================================================================


// --- Настройки ---
const float RESISTOR_OHMS = 150.6; // Сопротивление нашего резистора в Омах

// Характеристики датчика 4-20 мА
const float CURRENT_MIN_MA = 3.95;//4.0;
const float CURRENT_MAX_MA = 20.0;

// Диапазон измерения твоего датчика (например, 0-5 метров)
const float SENSOR_LEVEL_MIN_METERS = 0.0;
const float SENSOR_LEVEL_MAX_METERS = 10.0; // Укажи максимальную глубину для твоей модели


// это число умножить на 2 часа - интервал между передачей на сервер данных
#define DAILY_INTERVAL 3

#define MAX_LINES_TO_SEND 10