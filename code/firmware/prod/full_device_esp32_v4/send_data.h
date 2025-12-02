
#include <Arduino.h>

bool attemptToSend(String messageText, uint32_t failCounter);

bool sendPayloadToServer(const String& payload);

void modemOn();

void modemOff();