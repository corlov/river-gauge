#ifndef TYPES_H_
#define TYPES_H_

#include <Arduino.h>

struct PrevState {
  uint32_t bootCount;
  bool success;
  uint32_t failCounter;
};


const int ERR_CODE_RTC = 1;
const int ERR_CODE_BME = 2;
const int ERR_CODE_INA219 = 3;
const int ERR_CODE_WATER_TEMP = 4;
const int ERR_CODE_WATER_LEVEL_ERROR = 5;
const int ERR_CODE_FS_CORRUPT = 6;
const int ERR_CODE_SEND_ERROR = 7;
const int ERR_CODE_RESPONSE = 8;
const int ERR_CODE_UPDATE_RTC = 9;
#endif
  