const int ERR_WRITE_LOG_FILE         =   1;
const int ERR_CODE_FS_CORRUPT        =   2;
// 3
const int ERR_CODE_RTC               =  11;
const int ERR_CODE_BME               =  12;
const int ERR_CODE_INA219            =  13;
const int ERR_CODE_SEND_ERROR        =  21;
const int ERR_CODE_RESPONSE          =  22;
const int ERR_CODE_UPDATE_RTC        =  23;
const int ERR_OPEN_LOG               =  33;

const int ERR_CODE_WATER_TEMP        = 111;
const int ERR_CODE_WATER_LEVEL_ERROR = 112;
const int ERR_OPEN_LOG_FILE_2        = 113;
const int ERR_CODE_UPDATE_RTC_PARSE  = 121;
const int ERR_SMS                    = 122;
const int ERR_MODEM_RESTART          = 123;
const int ERR_MODEM_NET_NOT_FOUND    = 131;
const int ERR_MODEM_GPRS_CONNECT     = 132;
const int ERR_SMS_SEND               = 133;

const int ERR_MQTT_CONNECT           = 211;
const int ERR_MQTT_APPLY_CONFIG      = 212;
const int ERR_PUBLISH_MSG            = 213;
const int ERR_MQTT_CONNECT_2         = 221;
const int ERR_HTTP_CONNECT           = 222;
const int ERR_HTTP_EMPTY_PKG         = 223;
// 231
// 232
// 233
// 311
// ...
// 333


void blinkErrorCode(int errorCode);