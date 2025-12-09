#include <Arduino.h>

#define MQTT_TRANSMIT_TYPE 1
//#define HTTP_TRANSMIT_TYPE 2


#define FIRMWARE_VERSION "1.0.0"
#define INSTALL_DATE "01.12.2025"
#define SETTING_DEVICE_ID "device_id"
#define DEFAULT_DEVICE_ID 7001
#define GPS_LON 56.06522 
#define GPS_LAT 36.23429

const char apn[] = "internet";
const char gprs_user[] = "";
const char gprs_pass[] = "";

// --- ОСНОВНОЙ АДРЕС ---
const char primary_server_address[] = "89.169.3.241";
const uint16_t primary_server_port = 8001;

// --- РЕЗЕРВНЫЙ АДРЕС ---
const char secondary_server_address[] = "89.169.3.241";
const uint16_t secondary_server_port = 8001;

#define LED_PIN                         LED_BUILTIN
#define DONE_PIN                        4
#define WATER_TEMPERATURE_SENSOR_PIN    5
#define ERRORE_LED_PIN                  6
#define DEBUG_LED_PIN                   7
#define MODEM_POWER_PIN                 12
#define WATER_LEVEL_SENSOR_PIN          14
#define MODEM_RX_PIN                    15
#define MODEM_TX_PIN                    16

const int unusedPins[] = {0,1,2,3,/*4, 5, 6, 7, */ 8,9,10,11,/*12,*/13,/*14,15,16,*/
                17,18,19,20,21, 35,40,41,42,/*43,44,*/45,46,47/*,48*/};

// Характеристики датчика уровня 4-20 мА после калибровки
const float CURRENT_MAX_MA = 20.0;
const float SENSOR_LEVEL_MIN_METERS = 0.0;
const float SENSOR_LEVEL_MAX_METERS = 10.0;


#define MAX_LINES_TO_SEND 10

// Количество считываний для получения медианы
const int NUM_READINGS = 13;

// после такого числа неуспешных отправок шлем СМС специальное
const int MAX_FAILS_SEND_COUNT = 5;


#define SETTING_PHONE "phone"
#define DEFAULT_PHONE_NUMBER "+79055191010"

#define SETTING_ACTIVATION_FREQ "activation_freq"
#define DEFAULT_MODEM_ACTIVATION_FREQENCY 2

#define SETTING_CURRENT_MIN_MA "min_current"
#define DEFAULT_CURRENT_MIN_MA 3.95 // по-умолчанию 4.0;

#define SETTING_RESISTOR_OHMS "resistor" // сопротивление шунтирующего резистора в Омах около датчика уровня воды
#define DEFAULT_RESISTOR_OHMS 150.6 // максимум 165 Ом, но не более!!!






#define SETTING_TRANSPORT_TYPE "transport_type"
#define TRANSPORT_HTTP 0
#define TRANSPORT_MQTT 1

#define SETTING_MQTT_HOST "mqtt_host"
#define SETTING_MQTT_PORT "mqtt_port"
#define SETTING_MQTT_USER "mqtt_user"
#define SETTING_MQTT_PASS "mqtt_pass"

#define DEFAULT_MQTT_HOST "89.169.3.241" 
#define DEFAULT_MQTT_PORT 1883
#define DEFAULT_MQTT_USER "device_user"
#define DEFAULT_MQTT_PASS "sm191DY1oN5TDxMz"


// TODO: резервный адрес отправки по MQTT


#define GSM_WAIT_TIMEOUT 70000L


const uint WAITING_SETTINGS_REQ_TIMEOUT = 5000;

#define JSON_BUFF_SIZE 512