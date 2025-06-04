#include "ota_task.h"
#include "mqtt_task.h"
#include "ap_mode_task.h"
#include "wifi_task.h"

// OTA configuration
const char CURRENT_FIRMWARE_TITLE[] = "OTA Project";
const char CURRENT_FIRMWARE_VERSION[] = "2.0";
const uint8_t FIRMWARE_FAILURE_RETRIES = 12U;
const uint16_t FIRMWARE_PACKET_SIZE = 4096U;

// Global variables
Espressif_Updater<> updater;
bool currentFWSent = false;
bool updateRequestSent = false;

void update_starting_callback() {
  // This callback is called when update starts
}

void finished_callback(const bool &success) {
  if (success) {
    Serial.println("Done, Reboot now");
    esp_restart();
    return;
  }
  Serial.println("Downloading firmware failed");
}

void progress_callback(const size_t &current, const size_t &total) {
  Serial.printf("Progress %.2f%%\n", static_cast<float>(current * 100U) / total);
}

// OTA Update Task
void otaTask(void *parameter) {
  while (true) {
    // Skip if in AP mode
    if (apMode) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    
    // Wait for MQTT connection
    if (!mqttConnected) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    
    if (xSemaphoreTake(tbMutex, portMAX_DELAY) == pdTRUE) {
      if (!currentFWSent) {
        currentFWSent = ota.Firmware_Send_Info(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION);
      }
      
      if (!updateRequestSent) {
        Serial.print(CURRENT_FIRMWARE_TITLE);
        Serial.println(CURRENT_FIRMWARE_VERSION);
        Serial.println("Firmware Update ...");
        const OTA_Update_Callback callback(CURRENT_FIRMWARE_TITLE, CURRENT_FIRMWARE_VERSION, &updater, &finished_callback, &progress_callback, &update_starting_callback, FIRMWARE_FAILURE_RETRIES, FIRMWARE_PACKET_SIZE);
        updateRequestSent = ota.Start_Firmware_Update(callback);
        if (updateRequestSent) {
          delay(500);
          Serial.println("Firmware Update Subscription...");
          updateRequestSent = ota.Subscribe_Firmware_Update(callback);
        }
      }
      
      xSemaphoreGive(tbMutex);
    }
    
    vTaskDelay(10000 / portTICK_PERIOD_MS); // Check OTA less frequently
  }
} 