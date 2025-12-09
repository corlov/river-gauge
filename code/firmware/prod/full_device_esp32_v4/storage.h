#include "water_lvl_types.h"
#include "FS.h"
#include "LittleFS.h"
#include <vector>


#define FLASH_STORAGE_NAME "device-storage"

#define SETTINGS_NAMESPACE "settings"

#define LOG_FILE_PATH "/datalog.csv"

PrevState loadAndIncrementBootState();

void addCsvLine(const String& csvData);

void setSuccess(bool val);

String prepareLogPayload();



String readStringSetting(const char* key, const char* defaultValue);

void writeStringSetting(const char* key, const String& value);

int readIntSetting(const char* key, int defaultValue);

void writeIntSetting(const char* key, int value);

float readFloatSetting(const char* key, float defaultValue);

void writeFloatSetting(const char* key, float value);


std::vector<String> prepareLogPayloadAsArray();


