#include "fan_task.h"
#include "mqtt_task.h"
#include "ap_mode_task.h"
#include "wifi_task.h"
#include "sinric_task.h"

void setupFanPWM() {
  // Configure PWM for fan control
  ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ, FAN_PWM_RESOLUTION);
  ledcAttachPin(FAN_PIN, FAN_PWM_CHANNEL);
  ledcWrite(FAN_PWM_CHANNEL, FAN_PWM_SPEED_0); // Start with fan off
}

void updateFanSpeed(bool state) {
  // Check if LEDC is initialized, if not initialize it
  static bool pwmInitialized = false;
  if (!pwmInitialized) {
    setupFanPWM();
    pwmInitialized = true;
    Serial.println("Fan PWM initialized on-demand");
  }
  
  // Set fan speed based on state (50% when on, 0% when off)
  ledcWrite(FAN_PWM_CHANNEL, state ? FAN_PWM_SPEED_50 : FAN_PWM_SPEED_0);
  //Serial.printf("Fan PWM set to %d%%\n", state ? 50 : 0);
}

void fanTask(void *parameter) {
  // Make sure PWM is initialized at task start
  static bool pwmInitialized = false;
  if (!pwmInitialized) {
    setupFanPWM();
    pwmInitialized = true;
    Serial.println("Fan PWM initialized from task");
  }
  
  while (true) {
    // Skip if in AP mode
    if (apMode) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    
    // Update fan speed based on current state
    updateFanSpeed(fanState);
    
    vTaskDelay(100 / portTICK_PERIOD_MS); // Check every 100ms
  }
}