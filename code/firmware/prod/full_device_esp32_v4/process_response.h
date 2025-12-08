#include <ArduinoJson.h> 


bool updateRtcFromServerTime(const char* timeString);

bool processServerResponse(const String& responseBody);

void settingsCallback(char* topic, byte* payload, unsigned int length);

