#ifndef TYPES_H_
#define TYPES_H_

#include <Arduino.h>

struct PrevState {
  uint32_t bootCount;
  bool success;
  uint32_t failCounter;
};


struct MqttConfig {
  const char* host;
  int port;
  const char* user;
  const char* pass;
};


#endif
  