#include "mqtt_task.h"
#include "wifi_task.h"
#include "ap_mode_task.h"
#include "sinric_task.h"

// MQTT configuration
const char TOKEN[] = "aGit27ekFWZqBqDWxL21";
const char THINGSBOARD_SERVER[] = "app.coreiot.io";
const uint16_t THINGSBOARD_PORT = 1883U;
const uint16_t MAX_MESSAGE_SEND_SIZE = 512U;
const uint16_t MAX_MESSAGE_RECEIVE_SIZE = 512U;

// Shared Attributes Configuration
const uint8_t MAX_ATTRIBUTES = 5U;
const std::array<const char*, MAX_ATTRIBUTES> SHARED_ATTRIBUTES = 
{
  "deviceState1",
  "deviceState2",
  "deviceState3",
  "deviceState4",
  "deviceState5"
};

// RPC Methods
const char RPC_LED_CONTROL[] = "set_led";
const char RPC_FAN_CONTROL[] = "set_fan";
const char RPC_SWITCH_METHOD[] = "set_switch";
const uint8_t MAX_RPC_SUBSCRIPTIONS = 3U;
const uint8_t MAX_RPC_RESPONSE = 5U;
const uint64_t REQUEST_TIMEOUT_MICROSECONDS = 10000U * 1000U;

// Global variables
bool mqttConnected = false;
bool shared_update_subscribed = false;
bool requestedShared = false;
bool rpc_subscribed = false;
bool ledState = false;
bool fanState = false;
bool lastKnownLedState = false; // Track last known state to prevent loops
bool lastKnownFanState = false;
unsigned long lastSharedRequest = 0;
const unsigned long SHARED_REQUEST_INTERVAL = 5000; // Request every 5 seconds to avoid server override conflicts
bool forceSharedRequest = false; // Flag to force immediate request

// Debug print control
static unsigned long lastDebugPrint = 0;
const unsigned long DEBUG_PRINT_INTERVAL = 10000; // Print debug info every 10 seconds max

// GPIO pins - need to be defined here for RPC callbacks
// #define LED_PIN 2
// #define FAN_PIN 3

// Initialize underlying client, used to establish a connection
WiFiClient espClient;
// Initialize the Mqtt client instance
Arduino_MQTT_Client mqttClient(espClient);
// Initialize used apis
OTA_Firmware_Update<> ota;
Shared_Attribute_Update<1U, MAX_ATTRIBUTES> shared_update;
Attribute_Request<2U, MAX_ATTRIBUTES> attr_request;
Server_Side_RPC<MAX_RPC_SUBSCRIPTIONS, MAX_RPC_RESPONSE> rpc;
const std::array<IAPI_Implementation*, 4U> apis = {
    &shared_update,
    &attr_request,
    &ota,
    &rpc
};
// Initialize ThingsBoard instance with the maximum needed buffer size
ThingsBoard tb(mqttClient, MAX_MESSAGE_RECEIVE_SIZE, MAX_MESSAGE_SEND_SIZE, Default_Max_Stack_Size, apis);

void requestTimedOut() {
  Serial.printf("Attribute request timed out did not receive a response in (%llu) microseconds. Ensure client is connected to the MQTT broker and that the keys actually exist on the target device\n", REQUEST_TIMEOUT_MICROSECONDS);
}

// LED Control
void processLedControl(const JsonVariantConst &data, JsonDocument &response) {
  if (data.containsKey("value")) {
    bool led_value = data["value"];
    ledState = led_value;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    Serial.printf("RPC LED control: %s\n", ledState ? "ON" : "OFF");
    
    // Send attribute update to ThingsBoard
    if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
      tb.sendAttributeData("deviceState1", ledState);
      xSemaphoreGive(tbMutex);
    } else {
      Serial.println("RPC LED: Failed to acquire tbMutex");
    }
    
    // Update SinricPro (only if WiFi connected and not in AP mode)
    if (wifiConnected && !apMode) {
      updateSinricProState(ledState);
    }
    
    response["status"] = "success";
    response["value"] = ledState;
  } else {
    response["status"] = "error";
    response["error"] = "No value provided";
  }
}

// Fan Control
void processFanControl(const JsonVariantConst &data, JsonDocument &response) {
  if (data.containsKey("value")) {
    bool fan_value = data["value"];
    fanState = fan_value;
    lastKnownFanState = fan_value; // Update tracking to prevent loops
    digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
    Serial.printf("RPC Fan control: %s\n", fanState ? "ON" : "OFF");
    
    // Send attribute update to ThingsBoard
    if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
      tb.sendAttributeData("deviceState2", fanState);
      xSemaphoreGive(tbMutex);
    } else {
      Serial.println("RPC Fan: Failed to acquire tbMutex");
    }
    
    response["status"] = "success";
    response["value"] = fanState;
  } else {
    response["status"] = "error";
    response["error"] = "No value provided";
  }
}

// Generic Switch Control
void processSwitchControl(const JsonVariantConst &data, JsonDocument &response) {
  if (data.containsKey("switch")) {
    bool switch_value = data["switch"];
    // Switch can control either LED or Fan based on context
    // For this example, we'll control the LED
    ledState = switch_value;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    Serial.printf("RPC Switch control: %s\n", ledState ? "ON" : "OFF");
    
    // Send attribute update to ThingsBoard
    if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
      tb.sendAttributeData("deviceState1", ledState);
      xSemaphoreGive(tbMutex);
    } else {
      Serial.println("RPC Switch: Failed to acquire tbMutex");
    }
    
    // Update SinricPro (only if WiFi connected and not in AP mode)
    if (wifiConnected && !apMode) {
      updateSinricProState(ledState);
    }
    
    response["status"] = "success";
    response["value"] = ledState;
  } else {
    response["status"] = "error";
    response["error"] = "No switch value provided";
  }
}

void processSharedAttributeUpdate(const JsonObjectConst &data) {
  bool hasActualChange = false;
  
  for (auto it = data.begin(); it != data.end(); ++it) {
    const char* key = it->key().c_str();
    
    // Handle different attribute types
    if (strcmp(key, "deviceState1") == 0) {
      bool newLedState = it->value().as<bool>();
      
      // Only accept updates that represent real changes from dashboard
      if (newLedState != lastKnownLedState) {
        ledState = newLedState;
        lastKnownLedState = newLedState;
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);
        hasActualChange = true;
        
        Serial.printf("Dashboard changed LED to: %s\n", ledState ? "ON" : "OFF");
        
        // Update SinricPro with the new state (only if WiFi connected and not in AP mode)
        if (wifiConnected && !apMode) {
          updateSinricProState(ledState);
          Serial.printf("Synced to Google Home: %s\n", ledState ? "ON" : "OFF");
        }
      }
    } 
    else if (strcmp(key, "deviceState2") == 0) {
      bool newFanState = it->value().as<bool>();
      
      // Only accept updates that represent real changes from dashboard
      if (newFanState != lastKnownFanState) {
        fanState = newFanState;
        lastKnownFanState = newFanState;
        digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
        hasActualChange = true;
        
        Serial.printf("Dashboard changed Fan to: %s\n", fanState ? "ON" : "OFF");
      }
    }
    else if (strcmp(key, "deviceState3") == 0) {
      bool state3 = it->value().as<bool>();
      // Only print if implementing device 3 control
    }
    else if (strcmp(key, "deviceState4") == 0) {
      bool state4 = it->value().as<bool>();
      // Only print if implementing device 4 control
    }
    else if (strcmp(key, "deviceState5") == 0) {
      bool state5 = it->value().as<bool>();
      // Only print if implementing device 5 control
    }
  }
}

void processSharedAttributeRequest(const JsonObjectConst &data) {
  static bool lastServerOverrideState = false;
  static bool lastServerOverrideStateFan = false;
  bool hasStateChange = false;
  bool hasServerOverride = false;
  
  // Process the shared attributes the same way as updates
  for (auto it = data.begin(); it != data.end(); ++it) {
    const char* key = it->key().c_str();
    
    // Handle different attribute types
    if (strcmp(key, "deviceState1") == 0) {
      bool newLedState = it->value().as<bool>();
      
      // Check if server state conflicts with current state
      if (newLedState != lastKnownLedState) {
        hasServerOverride = true;
        
        // Only print if this is a new override situation
        if (!lastServerOverrideState) {
          Serial.printf("SERVER OVERRIDE DETECTED - Server wants %s but device is %s\n", 
                       newLedState ? "ON" : "OFF", 
                       lastKnownLedState ? "ON" : "OFF");
          lastServerOverrideState = true;
        }
        
        // Instead of accepting server override, update server with current state
        if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
          tb.sendAttributeData("deviceState1", lastKnownLedState);
          xSemaphoreGive(tbMutex);
        }
      } else {
        // States match - reset override flag
        if (lastServerOverrideState) {
          Serial.println("Server state corrected - states now match");
          lastServerOverrideState = false;
        }
      }
    } 
    else if (strcmp(key, "deviceState2") == 0) {
      bool newFanState = it->value().as<bool>();
      
      // Check if server state conflicts with current state
      if (newFanState != lastKnownFanState) {
        hasServerOverride = true;
        
        // Only print if this is a new override situation for Fan
        if (!lastServerOverrideStateFan) {
          Serial.printf("SERVER OVERRIDE DETECTED (Fan) - Server wants %s but device is %s\n", 
                       newFanState ? "ON" : "OFF", 
                       lastKnownFanState ? "ON" : "OFF");
          lastServerOverrideStateFan = true;
        }
        
        // Instead of accepting server override, update server with current state
        if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
          tb.sendAttributeData("deviceState2", lastKnownFanState);
          xSemaphoreGive(tbMutex);
        }
      } else {
        // States match - reset override flag for Fan
        if (lastServerOverrideStateFan) {
          Serial.println("Server state corrected (Fan) - states now match");
          lastServerOverrideStateFan = false;
        }
      }
    }
    else if (strcmp(key, "deviceState3") == 0) {
      bool state3 = it->value().as<bool>();
      // Only print if implementing device 3 control
    }
    else if (strcmp(key, "deviceState4") == 0) {
      bool state4 = it->value().as<bool>();
      // Only print if implementing device 4 control
    }
    else if (strcmp(key, "deviceState5") == 0) {
      bool state5 = it->value().as<bool>();
      // Only print if implementing device 5 control
    }
  }
}

void mqttTask(void *parameter) {
  while (true) {
    // Skip if in AP mode
    if (apMode) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    
    // Wait for WiFi connection
    if (!wifiConnected) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    
    if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
      if (!tb.connected()) {
        // Reconnect to the ThingsBoard server,
        // if a connection was disrupted or has not yet been established
        Serial.printf("Connecting to: (%s) with token (%s)\n", THINGSBOARD_SERVER, TOKEN);
        mqttConnected = tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT);
        if (!mqttConnected) {
          Serial.println("Failed to connect to MQTT");
          xSemaphoreGive(tbMutex);
          vTaskDelay(5000 / portTICK_PERIOD_MS); // Retry after 5 seconds
          continue;
        }
        
        if (!requestedShared) {
          Serial.println("Requesting initial shared attributes...");
          const Attribute_Request_Callback<MAX_ATTRIBUTES> sharedCallback(&processSharedAttributeRequest, REQUEST_TIMEOUT_MICROSECONDS, &requestTimedOut, SHARED_ATTRIBUTES);
          requestedShared = attr_request.Shared_Attributes_Request(sharedCallback);
          if (!requestedShared) {
            Serial.println("Failed to request shared attributes");
          } else {
            Serial.println("Initial shared attributes request sent successfully");
          }
        }

        if (!shared_update_subscribed) {
          Serial.println("Subscribing for shared attribute updates...");
          const Shared_Attribute_Callback<MAX_ATTRIBUTES> callback(&processSharedAttributeUpdate, SHARED_ATTRIBUTES);
          if (!shared_update.Shared_Attributes_Subscribe(callback)) {
            Serial.println("Failed to subscribe for shared attribute updates");
          }
          Serial.println("Subscribe done");
          shared_update_subscribed = true;
        }
        
        if (!rpc_subscribed) {
          Serial.println("Subscribing for RPC...");
          const std::array<RPC_Callback, MAX_RPC_SUBSCRIPTIONS> callbacks = {
            RPC_Callback{ RPC_LED_CONTROL, processLedControl },
            RPC_Callback{ RPC_FAN_CONTROL, processFanControl },
            RPC_Callback{ RPC_SWITCH_METHOD, processSwitchControl }
          };
          
          if (!rpc.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
            Serial.println("Failed to subscribe for RPC");
          } else {
            Serial.println("RPC Subscribe done");
            rpc_subscribed = true;
          }
        }
      }
      
      // Immediate or periodic shared attribute request to ensure sync
      unsigned long currentTime = millis();
      bool shouldRequest = forceSharedRequest || (currentTime - lastSharedRequest > SHARED_REQUEST_INTERVAL);
      
      if (shouldRequest) {
        if (forceSharedRequest) {
          Serial.println("Force sync request (button/SinricPro change)");
          forceSharedRequest = false; // Reset flag
        }
        
        const Attribute_Request_Callback<MAX_ATTRIBUTES> callback(&processSharedAttributeRequest, REQUEST_TIMEOUT_MICROSECONDS, &requestTimedOut, SHARED_ATTRIBUTES);
        attr_request.Shared_Attributes_Request(callback);
        lastSharedRequest = currentTime;
      }
      
      // Process MQTT messages
      tb.loop();
      
      // Removed automatic force request to reduce noise
      
      xSemaphoreGive(tbMutex);
    }
    
    vTaskDelay(100 / portTICK_PERIOD_MS); // Process MQTT frequently
  }
} 

// Function to send RFID data to ThingsBoard
void sendRfidData(const String &cardId) {
  // Only send if MQTT is connected and not in AP mode
  if (!mqttConnected || apMode || !wifiConnected) {
    return;
  }
  
  if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    // Send RFID card ID as telemetry data using individual key-value pairs
    bool sent1 = tb.sendTelemetryData("rfid_card_id", cardId.c_str());
    bool sent2 = tb.sendTelemetryData("rfid_timestamp", millis());
    
    if (sent1 && sent2) {
      Serial.printf("RFID data sent to ThingsBoard: %s\n", cardId.c_str());
    } else {
      Serial.println("Failed to send RFID data to ThingsBoard");
    }
    
    xSemaphoreGive(tbMutex);
  } else {
    Serial.println("Failed to acquire tbMutex for RFID data");
  }
} 