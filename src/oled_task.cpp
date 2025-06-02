#include "oled_task.h"
#include "wifi_task.h" // Đảm bảo wifi_task.h có khai báo extern bool wifiConnected;
#include <main_constants.h> // Để dùng SCREEN_WIDTH, SCREEN_HEIGHT, etc.
// Khởi tạo đối tượng display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Hàm khởi tạo OLED
void setupOLED() {
    // Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); // Gọi 1 lần trong main_setup() là đủ
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        // Không nên loop forever ở đây trong môi trường RTOS, chỉ log lỗi
        return;
    }
    Serial.println(F("SSD1306 Initialized"));
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("OLED Ready!");
    display.display();
    delay(1000); // Chờ một chút để user thấy
}

// Hàm đồng bộ thời gian NTP (gọi khi có WiFi)
void initNTP() {
    if (wifiConnected) {
        Serial.println("Configuring NTP time...");
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        struct tm timeinfo;
        if(!getLocalTime(&timeinfo)){
            Serial.println("Failed to obtain NTP time");
            return;
        }
        Serial.println("NTP time synchronized");
        // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S"); // In thời gian đã đồng bộ
    } else {
        Serial.println("NTP sync skipped: WiFi not connected.");
    }
}

// Task chính cho OLED
void oled_task(void *parameter) {
    struct tm timeinfo;
    char timeString[10]; // HH:MM:SS + null
    char dateString[12]; // DD/MM/YYYY + null

    while (true) {
        display.clearDisplay();
        display.setTextSize(1); // Font size nhỏ cho ngày tháng
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);

        if (wifiConnected && getLocalTime(&timeinfo, 500)) { // Thêm timeout nhỏ cho getLocalTime
            // Hiển thị ngày tháng
            sprintf(dateString, "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
            display.println(dateString);

            // Hiển thị thời gian với font lớn hơn
            display.setTextSize(2); // Font size lớn cho giờ:phút:giây
            display.setCursor(0, 10); // Điều chỉnh vị trí cho phù hợp
            sprintf(timeString, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            display.println(timeString);

        } else {
            display.setTextSize(1);
            display.setCursor(0,0);
            if (!wifiConnected) {
                display.println("WiFi Connecting...");
            } else {
                display.println("Time Syncing...");
            }
        }
        display.display();
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Cập nhật mỗi giây
    }
}