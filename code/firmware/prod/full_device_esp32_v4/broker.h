#include "pb_encode.h"
#include "telemetry.pb.h"
#include <Arduino.h>
#include <vector>



bool fetchMqttSettings();

void parseCsvAndFillProtobuf(const String& csv, iot_telemetry_TelemetryData* message);

size_t prepareProtobufPayload(const String& csv_payload, uint8_t* buffer, size_t buffer_size);

bool sendMqttMessage(uint8_t* payload, unsigned int length);

bool attemptToSendMqtt(const String& csv_payload, const std::vector<String>& logLines);
