#ifndef MAIN_CONSTANTS_H
#define MAIN_CONSTANTS_H

// EEPROM configuration
extern const int EEPROM_SIZE;
extern const int SSID_ADDR;
extern const int PASS_ADDR;
extern const int ESP_MAX_SSID_LEN;
extern const int ESP_MAX_PASS_LEN;

// Cấu hình OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels (phổ biến là 64 hoặc 32)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // Địa chỉ I2C của OLED (thường là 0x3C hoặc 0x3D)

// GPIO Pins for control
#define LED_PIN 2  // LED pin
#define FAN_PIN 3  // Fan pin
#define AP_MODE_PIN 0  // Button pin for forcing AP mode
#define I2C_SDA_PIN GPIO_NUM_11
#define I2C_SCL_PIN GPIO_NUM_12
// Cấu hình NTP
extern const char* ntpServer;
extern const long  gmtOffset_sec;
extern const int   daylightOffset_sec;

#endif /* MAIN_CONSTANTS_H */ 