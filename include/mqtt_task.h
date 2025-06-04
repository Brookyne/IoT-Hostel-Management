#ifndef MQTT_TASK_H
#define MQTT_TASK_H

#include <Arduino.h>
#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include <Shared_Attribute_Update.h>
#include <Attribute_Request.h>
#include <Server_Side_RPC.h>
#include <OTA_Firmware_Update.h>


// MQTT configuration
extern const char TOKEN[];
extern const char THINGSBOARD_SERVER[];
extern const uint16_t THINGSBOARD_PORT;

// Shared Attributes Configuration
extern const uint8_t MAX_ATTRIBUTES;
extern const std::array<const char*, 5> SHARED_ATTRIBUTES;

// RPC Methods
extern const char RPC_LED_CONTROL[];
extern const char RPC_FAN_CONTROL[];
extern const char RPC_SWITCH_METHOD[];
extern const uint8_t MAX_RPC_SUBSCRIPTIONS;
extern const uint8_t MAX_RPC_RESPONSE;

// Global variables
extern bool mqttConnected;
extern bool shared_update_subscribed;
extern bool requestedShared;
extern bool rpc_subscribed;
extern SemaphoreHandle_t tbMutex;
extern ThingsBoard tb;
extern OTA_Firmware_Update<> ota;
extern bool ledState;
extern bool fanState;
extern bool lastKnownLedState;
extern bool lastKnownFanState;
extern bool forceSharedRequest;

// Functions
void processLedControl(const JsonVariantConst &data, JsonDocument &response);
void processFanControl(const JsonVariantConst &data, JsonDocument &response);
void processSwitchControl(const JsonVariantConst &data, JsonDocument &response);
void processSharedAttributeUpdate(const JsonObjectConst &data);
void processSharedAttributeRequest(const JsonObjectConst &data);
void requestTimedOut();
void mqttTask(void *parameter);
void sendRfidData(const String &cardId);

#endif /* MQTT_TASK_H */ 