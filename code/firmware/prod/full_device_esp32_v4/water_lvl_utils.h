String getNthValue(const String& data, int index);

void modemOn();

void modemOff();

uint64_t stringToTimestamp(const String& dateTimeStr);

bool ensureGprsConnection();

uint64_t convertToTimestamp(int year, int month, int day, int hour, int min, int sec);