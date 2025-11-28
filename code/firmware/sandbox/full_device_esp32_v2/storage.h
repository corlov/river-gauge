#include "water_lvl_types.h"
#include "FS.h"
#include "LittleFS.h"


#define FLASH_STORAGE_NAME "device-storage"

#define LOG_FILE_PATH "/datalog.csv"

PrevState getPrevState();


void addCsvLine(const String& csvData);

//void processAndClearLogFile();

//bool sendDataToServer(const String& data);

void setSuccess(bool val);

String prepareLogPayload();