#include "button_task.h"
#include "mqtt_task.h"
#include "ap_mode_task.h"
#include "wifi_task.h"
#include "sinric_task.h"
#include "fan_task.h"
#include "Ticker.h"

// Button task configuration
Ticker buttonTicker;

void buttonTask() {
    getKeyInput();
}

void ButtonTask(void *pvParameters) {
  while (1) {
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck >= 100) { // Check every 100ms
      lastCheck = millis();
      for (int i = 0; i < NUM_BUTTONS; i++) {
        if (isButtonPressed(i)) {
          // Toggle LED state with button 0
          if (i == 0) {
            ledState = !ledState;
            lastKnownLedState = ledState; // Update tracking to prevent loops
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
            
            // Only update ThingsBoard - force server state update
            if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
              tb.sendAttributeData("deviceState1", ledState);
              xSemaphoreGive(tbMutex);
              Serial.printf("Button 0 pressed: LED %s (forced ThingsBoard server update)\n", ledState ? "ON" : "OFF");
            } else {
              Serial.println("ButtonTask: Failed to acquire tbMutex for LED state");
            }
            
            // Force immediate shared attribute request to trigger sync to SinricPro
            forceSharedRequest = true;
          }
          // Toggle Fan state with button 1
          else if (i == 1) {
            fanState = !fanState;
            lastKnownFanState = fanState; // Update tracking to prevent loops
            updateFanSpeed(fanState); // Use PWM control function
            
            if (xSemaphoreTake(tbMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
              tb.sendAttributeData("deviceState2", fanState);
              xSemaphoreGive(tbMutex);
            } else {
              Serial.println("ButtonTask: Failed to acquire tbMutex for Fan state");
            }
            Serial.printf("Button 1 pressed: Fan %s (forced ThingsBoard server update)\n", fanState ? "ON" : "OFF");
          }
        }
        if (isButtonLongPressed(i)) {
          // Long press button 0 to enter AP mode
          if (i == 0 && !apMode) {
            Serial.println("Long press detected. Entering AP mode for configuration...");
            setupAP();
          }
          Serial.printf("Button %d pressed (long)\n", i);
        }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
} 