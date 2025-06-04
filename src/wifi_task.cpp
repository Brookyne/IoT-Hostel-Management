#include "wifi_task.h"
#include <Arduino.h>
#include "ap_mode_task.h"

bool wifiConnected = false;
char WIFI_SSID[32] = "hehe";
char WIFI_PASSWORD[64] = "12345678";
const int WIFI_TIMEOUT = 20000;  // 20 seconds timeout for connection attempts
const int MAX_RECONNECT_ATTEMPTS = 3;  // Maximum number of reconnection attempts before entering AP mode

void InitWiFi() {
  Serial.println("Connecting to AP ...");
  // Attempting to establish a connection to the given WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Wait for connection with timeout
  uint32_t startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Connected to AP");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
  } else {
    Serial.println();
    Serial.println("Failed to connect to WiFi. Starting AP Mode for configuration...");
    // AP Mode setup will be called from main
  }
}

bool reconnect() {
  // Check to ensure we aren't connected yet
  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    wifiConnected = true;
    return true;
  }

  // If we aren't establish a new connection to the given WiFi network
  InitWiFi();
  return wifiConnected;
}

void wifiTask(void *parameter) {
  int reconnectAttempts = 0;
  
  while (true) {
    if (!(*((bool*)parameter))) {  // If not in AP mode
      if (!wifiConnected) {
        reconnect();
        
        // If still not connected, increment the attempts counter
        if (!wifiConnected) {
          reconnectAttempts++;
          Serial.printf("Reconnect attempt %d of %d failed\n", reconnectAttempts, MAX_RECONNECT_ATTEMPTS);
          
          // If we've reached the maximum number of attempts, enter AP mode
          if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
            Serial.println("Maximum reconnection attempts reached. Entering AP mode.");
            setupAP();
            reconnectAttempts = 0;  // Reset counter
          }
        } else {
          // Reset the counter if we successfully connected
          reconnectAttempts = 0;
        }
      }
      
      // Check WiFi status periodically
      if (WiFi.status() != WL_CONNECTED && wifiConnected) {
        Serial.println("WiFi connection lost, reconnecting...");
        wifiConnected = false;
      }
    } else {
      // Reset the counter when in AP mode
      reconnectAttempts = 0;
    }
    
    vTaskDelay(10000 / portTICK_PERIOD_MS); // Check every 10 seconds
  }
} 