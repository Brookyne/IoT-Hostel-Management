#ifndef OLED_TASK_H
#define OLED_TASK_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include "main_constants.h" // Để dùng SCREEN_WIDTH, SCREEN_HEIGHT, etc.

extern Adafruit_SSD1306 display;
extern bool wifiConnected; // Biến này cần được extern từ wifi_task.h

void setupOLED();
void oled_task(void *parameter);
void initNTP(); // Hàm đồng bộ NTP

#endif /* OLED_TASK_H */