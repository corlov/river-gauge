
#include <Arduino.h>

bool attemptToSendLogs(String messageText);

bool sendPayloadToServer(const String& payload);

void modemOn();

void modemOff();