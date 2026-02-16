#include <Arduino.h>



String gatherChannelInfo();

bool sendHttpRequest(const char* server, uint16_t port, const String& message);

bool sendPayloadWithFallback(const String& payloadLog, const String& payload);
