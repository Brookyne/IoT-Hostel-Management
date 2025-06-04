#include "oled_task.h"
#include "wifi_task.h" // Đảm bảo wifi_task.h có khai báo extern bool wifiConnected;
#include <main_constants.h> // Để dùng SCREEN_WIDTH, SCREEN_HEIGHT, etc.

// NTP settings definitions
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;    // GMT+7 (for Vietnam)
const int daylightOffset_sec = 0;       // No daylight saving time

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
        // Kiểm tra DNS và kết nối mạng
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi không còn kết nối. Không thể đồng bộ NTP.");
            return;
        }
        
        IPAddress dnsIP = WiFi.dnsIP();
        if (dnsIP == IPAddress(0,0,0,0)) {
            Serial.println("DNS không hợp lệ. Thử thiết lập DNS thủ công...");
            WiFi.config(WiFi.localIP(), WiFi.gatewayIP(), WiFi.subnetMask(), IPAddress(8,8,8,8));
            delay(1000);
        }

        Serial.println("Đang cấu hình NTP time...");
        // Thiết lập servers với IP tĩnh để tránh phụ thuộc vào DNS
        configTzTime("GMT-7", "time.google.com", "time.nist.gov", "pool.ntp.org");
        
        // Tăng thời gian chờ để đảm bảo có đủ thời gian lấy thông tin NTP
        Serial.println("Đang chờ đồng bộ NTP...");
        delay(2000);
        
        // Thử lấy thời gian với timeout lâu hơn và nhiều lần thử
        struct tm timeinfo;
        int retryCount = 0;
        const int maxRetries = 8;
        
        while (!getLocalTime(&timeinfo, 3000) && retryCount < maxRetries) {
            Serial.printf("Lấy thời gian NTP thất bại, đang thử lại (%d/%d)...\n", retryCount+1, maxRetries);
            retryCount++;
            // Thử ping để xác nhận kết nối mạng
            if (retryCount == 2) {
                Serial.println("Đang khởi động lại NTP client...");
                configTzTime("GMT-7", "1.asia.pool.ntp.org", "time.cloudflare.com", "pool.ntp.org");
            }
            delay(2000); // Thời gian chờ dài hơn
        }
        
        if (retryCount < maxRetries) {
            Serial.println("NTP time synchronized successfully!");
            char timeStringBuf[50];
            strftime(timeStringBuf, sizeof(timeStringBuf), "%A, %B %d %Y %H:%M:%S", &timeinfo);
            Serial.print("Current time: ");
            Serial.println(timeStringBuf);
        } else {
            Serial.println("Failed to obtain NTP time after multiple attempts");
        }
    } else {
        Serial.println("NTP sync skipped: WiFi not connected.");
    }
}

// Task chính cho OLED
void oled_task(void *parameter) {
    struct tm timeinfo;
    char timeString[10]; // HH:MM:SS + null
    char dateString[12]; // DD/MM/YYYY + null
    char dayOfWeekStr[12]; // Tên ngày trong tuần
    const char* daysOfWeek[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    bool colonVisible = true; // Dùng cho hiệu ứng nhấp nháy dấu hai chấm
    uint32_t lastTimeSync = 0;
    
    // Khởi tạo màn hình
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("Starting clock...");
    display.display();

    while (true) {
        display.clearDisplay();
        
        // Kiểm tra và đồng bộ lại thời gian mỗi 12 giờ
        if (wifiConnected && (millis() - lastTimeSync > 12 * 60 * 60 * 1000)) {
            initNTP();
            lastTimeSync = millis();
        }
        
        if (wifiConnected && getLocalTime(&timeinfo, 500)) {
            // Hiển thị tên ngày trong tuần
            display.setTextSize(1);
            display.setCursor(0, 0);
            strcpy(dayOfWeekStr, daysOfWeek[timeinfo.tm_wday]);
            display.println(dayOfWeekStr);
            
            // Hiển thị ngày tháng
            display.setCursor(0, 10);
            sprintf(dateString, "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
            display.println(dateString);
            
            // Hiển thị giờ:phút với hiệu ứng nhấp nháy dấu :
            display.setTextSize(2);
            display.setCursor(16, 24); // Căn giữa màn hình
            
            if (colonVisible) {
                sprintf(timeString, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
            } else {
                sprintf(timeString, "%02d %02d", timeinfo.tm_hour, timeinfo.tm_min);
            }
            display.println(timeString);
            
            // Hiển thị giây bằng font nhỏ
            display.setTextSize(1);
            display.setCursor(100, 30);
            sprintf(timeString, "%02d", timeinfo.tm_sec);
            display.println(timeString);
            
            // Đảo trạng thái dấu hai chấm
            colonVisible = !colonVisible;
        } else {
            display.setTextSize(1);
            display.setCursor(0, 0);
            if (!wifiConnected) {
                display.println("WiFi Connecting...");
                display.setCursor(0, 15);
                display.println("Please wait...");
            } else {
                display.println("Time Syncing...");
                display.setCursor(0, 15);
                display.println("Please wait...");
            }
        }
        
        // Hiển thị trạng thái wifi ở góc phải
        display.setCursor(SCREEN_WIDTH - 8, 0);
        display.setTextSize(1);
        display.println(wifiConnected ? "W" : "X");
        
        display.display();
        vTaskDelay(500 / portTICK_PERIOD_MS); // Cập nhật mỗi 0.5 giây để hiệu ứng nhấp nháy mượt
    }
}