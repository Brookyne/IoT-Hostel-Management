#include "google_sheets_task.h"
#include "wifi_task.h"
#include "main_constants.h"
#include <WiFiClientSecure.h>
#include <time.h>
#include <esp_task_wdt.h>

// Google Apps Script Configuration
const char* GOOGLE_SCRIPT_ID = "AKfycbzOipxPsgXpl_9QxJUuXGrtBVk2LmJcAg_lcUOaTrPIMKUeewvbRLDLZ6N6yn6Wy-4"; // Bạn sẽ thay thế sau
const char* GOOGLE_SCRIPT_URL = "https://script.google.com/macros/s/AKfycbzOipxPsgXpl_9QxJUuXGrtBVk2LmJcAg_lcUOaTrPIMKUeewvbRLDLZ6N6yn6Wy-4/exec";

// WiFi client for HTTPS
WiFiClientSecure client;

// In-memory database cho demo (trong thực tế có thể lưu vào SPIFFS hoặc EEPROM)
struct UserData {
  String cardId;
  String name;
};

// Database mẫu - sẽ được sync từ Google Sheets
UserData userDatabase[50]; // Tăng kích thước để chứa nhiều user hơn
int userDatabaseCount = 0;
unsigned long lastSyncTime = 0;
const unsigned long SYNC_INTERVAL = 300000; // Sync mỗi 5 phút

// Lưu trạng thái điểm danh cuối cùng
String lastAttendanceStatus[10]; // Giả sử tối đa 10 thẻ
String lastAttendanceCardId[10];
int attendanceCount = 0;

void setupGoogleSheets() {
  Serial.println("Setting up Google Sheets...");
  
  // Cấu hình NTP để lấy thời gian chính xác
  configTime(7 * 3600, 0, "pool.ntp.org"); // GMT+7 cho Việt Nam
  
  // Cấu hình SSL client (bỏ qua kiểm tra certificate cho demo)
  client.setInsecure();
  
  // ĐỢI WIFI VÀ THỰC HIỆN SYNC VỚI RETRY
  int wifiWaitCount = 0;
  while (!wifiConnected && wifiWaitCount < 30) { // Đợi tối đa 30 giây
    delay(1000);
    wifiWaitCount++;
  }
  
  if (!wifiConnected) {
    Serial.println("WiFi not connected, skipping sync");
    return;
  }
  
  // Retry initial sync 3 lần
  for (int i = 1; i <= 3; i++) {
    
    if (syncDatabaseFromGoogleSheets()) {
      return;
    } else {
      if (i < 3) {
        delay(5000);
      }
    }
  }
  
  Serial.println("Initial sync failed");
}

String getCurrentDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "Unknown";
  }
  
  char dateStr[20];
  char timeStr[20];
  
  strftime(dateStr, sizeof(dateStr), "%d/%m/%Y", &timeinfo);
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  
  return String(dateStr) + "," + String(timeStr);
}

String getUserNameFromCard(const String &cardId) {
  // Auto-sync nếu cần
  if (millis() - lastSyncTime > SYNC_INTERVAL) {
    Serial.println("🔄 Auto-sync (5min)...");
    syncDatabaseFromGoogleSheets();
  }
  
  for (int i = 0; i < userDatabaseCount; i++) {
    if (userDatabase[i].cardId == cardId) {
      return userDatabase[i].name;
    }
  }
  return "Unknown User";
}

bool isCardRegistered(const String &cardId) {
  // Auto-sync nếu cần
  if (millis() - lastSyncTime > SYNC_INTERVAL) {
    syncDatabaseFromGoogleSheets();
  }
  
  if (userDatabaseCount == 0) {
    Serial.println("Database empty, syncing...");
    if (syncDatabaseFromGoogleSheets()) {
      Serial.println("✅ Emergency sync OK");
    } else {
      Serial.println("Emergency sync failed");
      return false;
    }
  }
  
  for (int i = 0; i < userDatabaseCount; i++) {
    if (userDatabase[i].cardId == cardId) {
      return true;
    }
  }
  
  return false;
}

String determineAttendanceStatus(const String &cardId) {
  // Tìm trạng thái cuối cùng của thẻ này
  for (int i = 0; i < attendanceCount; i++) {
    if (lastAttendanceCardId[i] == cardId) {
      // Nếu lần cuối là "IN" thì lần này là "OUT", và ngược lại
      return (lastAttendanceStatus[i] == "IN") ? "OUT" : "IN";
    }
  }
  
  // Nếu chưa có record nào, mặc định là "IN"
  return "IN";
}

void updateLastAttendanceStatus(const String &cardId, const String &status) {
  // Tìm và cập nhật hoặc thêm mới
  for (int i = 0; i < attendanceCount; i++) {
    if (lastAttendanceCardId[i] == cardId) {
      lastAttendanceStatus[i] = status;
      return;
    }
  }
  
  // Thêm mới nếu chưa có
  if (attendanceCount < 10) {
    lastAttendanceCardId[attendanceCount] = cardId;
    lastAttendanceStatus[attendanceCount] = status;
    attendanceCount++;
  }
}

bool sendAttendanceToGoogleSheets(const AttendanceData &data) {
  if (!wifiConnected) {
    Serial.println("WiFi not connected");
    return false;
  }
  
  // Tạo JSON payload manually để tránh lỗi ArduinoJson
  String jsonString = "{";
  jsonString += "\"action\":\"addAttendance\",";
  jsonString += "\"cardId\":\"" + data.cardId + "\",";
  jsonString += "\"name\":\"" + data.name + "\",";
  jsonString += "\"status\":\"" + data.status + "\",";
  jsonString += "\"date\":\"" + data.date + "\",";
  jsonString += "\"time\":\"" + data.time + "\"";
  jsonString += "}";
  
  // Tạo HTTP client với timeout ngắn hơn
  HttpClient httpClient(client, "script.google.com", 443);
  httpClient.setHttpResponseTimeout(8000); // Giảm timeout xuống 8s
  
  // Tạo URL path
  String path = "/macros/s/" + String(GOOGLE_SCRIPT_ID) + "/exec";
  
  bool success = false;
  
  // Gửi POST request với error handling
  if (httpClient.post(path, "application/json", jsonString) == 0) {
    
    // Đọc response với timeout ngắn hơn
    unsigned long startTime = millis();
    while (!httpClient.available() && (millis() - startTime < 8000)) {
      delay(10);
    }
    
    if (httpClient.available()) {
      int statusCode = httpClient.responseStatusCode();
      
      // Chỉ đọc status, bỏ qua response body để tránh SSL error
      success = (statusCode == 200 || statusCode == 302);
      
      // Skip response body để tránh SSL read errors
      httpClient.skipResponseHeaders();
    }
  }
  
  // Proper cleanup
  httpClient.stop();
  delay(200); // Tăng delay cleanup để SSL connection đóng đúng cách
  
  return success;
}

void processRFIDForAttendance(const String &cardId) {
  Serial.printf("Card: %s\n", cardId.c_str());
  
  // Kiểm tra thẻ có đăng ký không
  if (!isCardRegistered(cardId)) {
    Serial.println("Card not registered");
    return;
  }
  
  // Lấy thông tin user
  String userName = getUserNameFromCard(cardId);
  String status = determineAttendanceStatus(cardId);
  String dateTime = getCurrentDateTime();
  
  // Tách date và time
  int commaIndex = dateTime.indexOf(',');
  String date = dateTime.substring(0, commaIndex);
  String time = dateTime.substring(commaIndex + 1);
  
  // Tạo attendance data
  AttendanceData attendance;
  attendance.cardId = cardId;
  attendance.name = userName;
  attendance.status = status;
  attendance.date = date;
  attendance.time = time;
  
  Serial.printf("Attendance: %s - %s\n", userName.c_str(), status.c_str());
  
  // Gửi lên Google Sheets
  if (sendAttendanceToGoogleSheets(attendance)) {
    Serial.println("✅ Sent to Google Sheets");
    updateLastAttendanceStatus(cardId, status);
  } else {
    Serial.println("Failed to send");
  }
}

void googleSheetsTask(void *parameter) {
  // Đợi WiFi kết nối
  while (!wifiConnected) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  
  setupGoogleSheets();
  
  // Task này chủ yếu chờ được gọi từ RFID task
  // Hoặc có thể sync dữ liệu định kỳ
  while (true) {
    // Có thể thêm logic sync định kỳ ở đây
    vTaskDelay(10000 / portTICK_PERIOD_MS); // Chờ 10 giây
  }
}

// Database sync functions
bool syncDatabaseFromGoogleSheets() {
  if (!wifiConnected) {
    Serial.println("WiFi not connected");
    return false;
  }
  
  Serial.println("Syncing database...");
  
  // THỬ PHƯƠNG PHÁP 1: GET REQUEST
  if (tryGetMethod()) {
    return true;
  }
  
  Serial.println("GET failed, trying POST...");
  
  // THỬ PHƯƠNG PHÁP 2: POST WITH FORM DATA
  return tryPostMethod();
}

bool tryGetMethod() {
  WiFiClientSecure httpClient;
  httpClient.setInsecure();
  httpClient.setTimeout(10000);
  
  String getPath = "/macros/s/" + String(GOOGLE_SCRIPT_ID) + "/exec?action=getDatabase";
  
  if (httpClient.connect("script.google.com", 443)) {
    
    httpClient.println("GET " + getPath + " HTTP/1.1");
    httpClient.println("Host: script.google.com");
    httpClient.println("User-Agent: ESP32-RFID/1.0");
    httpClient.println("Accept: application/json");
    httpClient.println("Connection: close");
    httpClient.println();
    
    unsigned long startTime = millis();
    while (!httpClient.available() && (millis() - startTime < 10000)) {
      delay(50);
      esp_task_wdt_reset();
    }
    
    if (httpClient.available()) {
      String statusLine = httpClient.readStringUntil('\n');
      
      // Skip headers
      bool headersDone = false;
      while (httpClient.available() && !headersDone) {
        String line = httpClient.readStringUntil('\n');
        if (line == "\r") {
          headersDone = true;
        }
        esp_task_wdt_reset();
      }
      
      String response = "";
      while (httpClient.available()) {
        response += httpClient.readString();
        esp_task_wdt_reset();
      }
      
      httpClient.stop();
      
      if (response.length() > 0) {
        bool success = parseDatabaseResponse(response);
        if (success) {
          lastSyncTime = millis();
          Serial.printf("✅ Database synced: %d users\n", userDatabaseCount);
          return true;
        }
      }
    }
    
    httpClient.stop();
  }
  
  return false;
}

bool tryPostMethod() {
  WiFiClientSecure httpClient;
  httpClient.setInsecure();
  httpClient.setTimeout(10000);
  
  String postPath = "/macros/s/" + String(GOOGLE_SCRIPT_ID) + "/exec";
  String formData = "action=getDatabase";
  
  if (httpClient.connect("script.google.com", 443)) {
    
    httpClient.println("POST " + postPath + " HTTP/1.1");
    httpClient.println("Host: script.google.com");
    httpClient.println("User-Agent: ESP32-RFID/1.0");
    httpClient.println("Content-Type: application/x-www-form-urlencoded");
    httpClient.println("Content-Length: " + String(formData.length()));
    httpClient.println("Connection: close");
    httpClient.println();
    httpClient.println(formData);
    
    unsigned long startTime = millis();
    while (!httpClient.available() && (millis() - startTime < 10000)) {
      delay(50);
      esp_task_wdt_reset();
    }
    
    if (httpClient.available()) {
      String statusLine = httpClient.readStringUntil('\n');
      
      // Skip headers
      bool headersDone = false;
      while (httpClient.available() && !headersDone) {
        String line = httpClient.readStringUntil('\n');
        if (line == "\r") {
          headersDone = true;
        }
        esp_task_wdt_reset();
      }
      
      String response = "";
      while (httpClient.available()) {
        response += httpClient.readString();
        esp_task_wdt_reset();
      }
      
      httpClient.stop();
      
      if (response.length() > 0) {
        bool success = parseDatabaseResponse(response);
        if (success) {
          lastSyncTime = millis();
          Serial.printf("✅ Database synced: %d users\n", userDatabaseCount);
          return true;
        }
      }
    }
    
    httpClient.stop();
  }
  
  return false;
}

bool parseDatabaseResponse(const String &response) {
  // Kiểm tra nếu response là HTML redirect
  if (response.indexOf("<HTML>") >= 0 && response.indexOf("Moved Temporarily") >= 0) {
    
    // Tìm redirect URL
    int hrefStart = response.indexOf("HREF=\"");
    if (hrefStart >= 0) {
      hrefStart += 6; // Skip "HREF=\""
      int hrefEnd = response.indexOf("\"", hrefStart);
      if (hrefEnd >= 0) {
        String redirectUrl = response.substring(hrefStart, hrefEnd);
        redirectUrl.replace("&amp;", "&");
        
        return followRedirectForJSON(redirectUrl);
      }
    }
    
    return false;
  }
  
  // Nếu không phải redirect, parse JSON trực tiếp
  return parseJSONResponse(response);
}

bool followRedirectForJSON(const String &redirectUrl) {
  // Extract domain và path từ redirect URL
  int protocolEnd = redirectUrl.indexOf("://");
  if (protocolEnd < 0) return false;
  
  int domainStart = protocolEnd + 3;
  int pathStart = redirectUrl.indexOf("/", domainStart);
  if (pathStart < 0) return false;
  
  String domain = redirectUrl.substring(domainStart, pathStart);
  String path = redirectUrl.substring(pathStart);
  
  // Tạo HTTP client cho redirect với timeout ngắn hơn
  WiFiClientSecure redirectClient;
  redirectClient.setInsecure();
  redirectClient.setTimeout(8000); // 8 giây
  
  if (redirectClient.connect(domain.c_str(), 443)) {
    
    redirectClient.println("GET " + path + " HTTP/1.1");
    redirectClient.println("Host: " + domain);
    redirectClient.println("Connection: close");
    redirectClient.println();
    
    // Đợi response với timeout ngắn
    unsigned long startTime = millis();
    while (!redirectClient.available() && (millis() - startTime < 8000)) {
      delay(50);
      esp_task_wdt_reset(); // Feed watchdog
    }
    
    if (redirectClient.available()) {
      // Skip headers
      bool headersDone = false;
      while (redirectClient.available() && !headersDone) {
        String line = redirectClient.readStringUntil('\n');
        if (line == "\r") {
          headersDone = true;
        }
        esp_task_wdt_reset();
      }
      
      // Đọc body
      String redirectResponse = "";
      while (redirectClient.available()) {
        redirectResponse += redirectClient.readString();
        esp_task_wdt_reset();
      }
      
      redirectClient.stop();
      
      if (redirectResponse.length() > 0) {
        return parseJSONResponse(redirectResponse);
      }
    }
    
    redirectClient.stop();
  }
  
  return false;
}

bool parseJSONResponse(const String &response) {
  // Tìm JSON data trong response
  int jsonStart = response.indexOf('{');
  int jsonEnd = response.lastIndexOf('}');
  
  if (jsonStart == -1 || jsonEnd == -1) {
    return false;
  }
  
  String jsonString = response.substring(jsonStart, jsonEnd + 1);
  
  // Debug JSON để thấy thay đổi từ database
  Serial.println("Database JSON:");
  Serial.println(jsonString);
  
  // Parse JSON manually (simple approach)
  if (jsonString.indexOf("\"status\":\"success\"") == -1) {
    return false;
  }
  
  // Reset database
  userDatabaseCount = 0;
  
  // Tìm users array
  int usersStart = jsonString.indexOf("\"users\":[");
  if (usersStart == -1) {
    
    // Check if it's explicitly empty
    if (jsonString.indexOf("\"users\":[]") >= 0) {
      return true;
    }
    
    // Check for count field
    if (jsonString.indexOf("\"count\":0") >= 0) {
      return true;
    }
    
    return false;
  }
  
  // Simple parsing cho từng user
  int pos = usersStart;
  while (pos < jsonString.length() && userDatabaseCount < 49) {
    
    // Tìm cardId
    int cardIdStart = jsonString.indexOf("\"cardId\":\"", pos);
    if (cardIdStart == -1) break;
    cardIdStart += 10; // Length of "cardId":"
    int cardIdEnd = jsonString.indexOf("\"", cardIdStart);
    if (cardIdEnd == -1) break;
    
    // Tìm name
    int nameStart = jsonString.indexOf("\"name\":\"", cardIdEnd);
    if (nameStart == -1) break;
    nameStart += 8; // Length of "name":"
    int nameEnd = jsonString.indexOf("\"", nameStart);
    if (nameEnd == -1) break;
    
    // Lưu vào database
    userDatabase[userDatabaseCount].cardId = jsonString.substring(cardIdStart, cardIdEnd);
    userDatabase[userDatabaseCount].name = jsonString.substring(nameStart, nameEnd);
    
    userDatabaseCount++;
    pos = nameEnd;
  }
  
  return true;
}

void printCurrentDatabase() {
  Serial.println("📋 Current User Database:");
  Serial.println("STT | Card ID  | Name");
  Serial.println("----|----------|------------------");
  
  for (int i = 0; i < userDatabaseCount; i++) {
    Serial.printf("%2d  | %-8s | %s\n", 
                  i+1, 
                  userDatabase[i].cardId.c_str(), 
                  userDatabase[i].name.c_str());
  }
  
  if (userDatabaseCount == 0) {
    Serial.println("    | (empty)  | No users found");
  }
  Serial.println("---------------------------");
}

unsigned long getLastSyncTime() {
  return lastSyncTime;
}

bool loadUserDatabaseFromSheets() {
  // Force sync from Google Sheets
  return syncDatabaseFromGoogleSheets();
} 