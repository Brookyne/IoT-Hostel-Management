#include "tasks.h"
#include "wifi_task.h"
#include "mqtt_task.h"
#include "sensor_task.h"
#include "ota_task.h"
#include "button_task.h"
#include "ap_mode_task.h"
#include "sinric_task.h"
#include "led_task.h"
#include "fan_task.h"
#include "rfid_task.h"
#include "google_sheets_task.h"
#include "oled_task.h"
// Task handles
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t otaTaskHandle = NULL;
TaskHandle_t buttonTaskHandle = NULL;
TaskHandle_t apModeTaskHandle = NULL;
TaskHandle_t sinricTaskHandle = NULL;
TaskHandle_t oledTaskHandle = NULL; // Add this line to define the handle
TaskHandle_t rfidTaskHandle = NULL;
TaskHandle_t googleSheetsTaskHandle = NULL;
TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t fanTaskHandle = NULL;
// Semaphore cho các thao tác ThingsBoard
SemaphoreHandle_t tbMutex = NULL;

// External variables
extern bool apMode;

// Tạo và khởi tạo semaphore
void initSemaphores() {
  tbMutex = xSemaphoreCreateMutex();
}

// Hàm khởi tạo tất cả các task
void createAllTasks() {
  // Tạo các task RTOS
  xTaskCreate(wifiTask, "WiFiTask", WIFI_TASK_STACK_SIZE, &apMode, WIFI_TASK_PRIORITY, &wifiTaskHandle);
  xTaskCreate(mqttTask, "MQTTTask", MQTT_TASK_STACK_SIZE, NULL, MQTT_TASK_PRIORITY, &mqttTaskHandle);
  xTaskCreate(sensorTask, "SensorTask", SENSOR_TASK_STACK_SIZE, NULL, SENSOR_TASK_PRIORITY, &sensorTaskHandle);
  xTaskCreate(otaTask, "OTATask", OTA_TASK_STACK_SIZE, NULL, OTA_TASK_PRIORITY, &otaTaskHandle);
  xTaskCreate(ButtonTask, "ButtonTask", BUTTON_TASK_STACK_SIZE, NULL, BUTTON_TASK_PRIORITY, &buttonTaskHandle);
  xTaskCreate(apModeTask, "APModeTask", AP_MODE_TASK_STACK_SIZE, NULL, AP_MODE_TASK_PRIORITY, &apModeTaskHandle);
  xTaskCreate(sinricTask, "SinricTask", SINRIC_TASK_STACK_SIZE, NULL, SINRIC_TASK_PRIORITY, &sinricTaskHandle);
  xTaskCreate(oled_task, "OLEDTask", OLED_TASK_STACK_SIZE, NULL, OLED_TASK_PRIORITY, &oledTaskHandle); // << THÊM MỚI
  xTaskCreate(rfidTask, "RFIDTask", RFID_TASK_STACK_SIZE, NULL, RFID_TASK_PRIORITY, &rfidTaskHandle);
  xTaskCreate(fanTask, "FanTask", FAN_TASK_STACK_SIZE, NULL, 1, &fanTaskHandle);
  xTaskCreate(ledTask, "LEDTask", LED_TASK_STACK_SIZE, NULL, 1, &ledTaskHandle);
  xTaskCreate(googleSheetsTask, "GoogleSheetsTask", GOOGLE_SHEETS_TASK_STACK_SIZE, NULL, GOOGLE_SHEETS_TASK_PRIORITY, &googleSheetsTaskHandle);

}
