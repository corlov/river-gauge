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
const char receiver_server_address[] = "94.228.123.208";
const int receiver_server_port = 8001;

// *********************************************
// аппаратные настройки пинов
// *********************************************
#define NEOPIXEL_PIN 48
#define MODEM_POWER_PIN 15
#define MODEM_RX_PIN 44
#define MODEM_TX_PIN 43
#define DONE_PIN 4
#define WATER_LEVEL_SENSOR_PIN A0
#define BATTERY_VOLTAGE_PIN A1
#define WATER_TEMPERATURE_SENSOR_PIN 5
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
const float VOLTAGE_DIVIDER_UP_RESISTOR = 33000.0; //17500.0;
const float VOLTAGE_DIVIDER_DOWN_RESISTOR = 10000.0; //10000.0;
// =============================================================================


// это число умножить на 2 часа - интервал между передачей на сервер данных
#define DAILY_INTERVAL 3

#define MAX_LINES_TO_SEND 10