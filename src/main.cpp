#include <Arduino.h>
#include <EEPROM.h>
#include <DHT.h>

#include "main_constants.h"
#include "tasks.h"
#include "wifi_task.h"
#include "mqtt_task.h"
#include "sensor_task.h"
#include "button_task.h"
#include "ap_mode_task.h"
#include "sinric_task.h"
#include "oled_task.h"
// Định nghĩa các hằng số
// const int EEPROM_SIZE = 512;  // Now defined in main_constants.cpp

// DHT sensor instance (declared in sensor_task.cpp)
extern DHT dht;

// External variables
extern bool apMode;

void setup() {
  // Khởi tạo kết nối serial
  Serial.begin(115200);
  Serial.println("RTOTA2");
  delay(1000);
  
  // Khởi tạo EEPROM và tải thông tin WiFi
  EEPROM.begin(EEPROM_SIZE);
  loadWiFiCredentials();
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  // Khởi tạo GPIO pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);
  
  setupOLED(); // << THÊM MỚI: Khởi tạo OLED

  // Khởi tạo cảm biến DHT
  dht.begin();
  
  // Khởi tạo semaphores
  initSemaphores();
  
  // Khởi tạo các pin nút nhấn
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  buttonTicker.attach_ms(10, buttonTask);

  // Kiểm tra nếu AP_MODE_PIN ở trạng thái LOW khi khởi động để bắt buộc vào chế độ AP
  if (digitalRead(AP_MODE_PIN) == LOW) {
    setupAP();
  } else {
    // Initialize WiFi and check for connection
    InitWiFi();
    if (wifiConnected) {
        initNTP(); // Đồng bộ NTP nếu WiFi kết nối thành công
    } else {
      Serial.println("Initial WiFi connection failed. Starting AP Mode...");
      setupAP();
    }
    // If WiFi connection failed, start AP mode
    if (!wifiConnected) {
      Serial.println("Initial WiFi connection failed. Starting AP Mode...");
      setupAP();
    }
  }
  
  

  // Tạo tất cả các task RTOS
  // createAllTasks();
}

void loop() {
  // Trống vì chức năng đã được chuyển sang các task RTOS
  vTaskDelay(1000 / portTICK_PERIOD_MS);
} 