#include <Arduino.h>
#include "rfid_task.h"
#include "tasks.h"
#include "mqtt_task.h"
#include "google_sheets_task.h"

// Khởi tạo đối tượng MFRC522
MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);

void setupRFID() {
  Serial.println(F("Khởi tạo RFID Reader..."));
  
  // Cấu hình các pin
  pinMode(RFID_SS_PIN, OUTPUT);
  pinMode(RFID_RST_PIN, OUTPUT);
  digitalWrite(RFID_SS_PIN, HIGH);
  digitalWrite(RFID_RST_PIN, HIGH);
  delay(10);
  
  // Khởi tạo SPI bus với pin cụ thể cho YoloUNO
  SPI.begin(RFID_SCK_PIN, RFID_MISO_PIN, RFID_MOSI_PIN, RFID_SS_PIN);
  SPI.setFrequency(1000000); // 1MHz
  
  // Reset MFRC522
  digitalWrite(RFID_RST_PIN, LOW);
  delay(50);
  digitalWrite(RFID_RST_PIN, HIGH);
  delay(100);
  
  // Khởi tạo MFRC522
  mfrc522.PCD_Init();
  delay(100);
  
  // Kiểm tra kết nối với nhiều lần thử và auto-retry
  Serial.println(F("Kiểm tra kết nối MFRC522..."));
  bool connected = false;
  const int MAX_INIT_RETRY = 10;
  
  for (int i = 0; i < MAX_INIT_RETRY; i++) {
    if (testRFIDConnection()) {
      connected = true;
      break;
    }
    
    Serial.printf("Init attempt %d/%d failed, retrying...\n", i+1, MAX_INIT_RETRY);
    
    // Reset và thử lại
    handleRFIDError();
    delay(500);
  }
  
  if (!connected) {
    Serial.println(F(" KHÔNG thể kết nối với MFRC522 sau nhiều lần thử!"));
    Serial.println(F("Hệ thống sẽ tiếp tục chạy và thử kết nối lại định kỳ"));
  } else {
    // Hiển thị thông tin về phần mềm
    Serial.println(F("✅ RFID Reader đã sẵn sàng!"));
    Serial.println(F("📱 Đưa thẻ gần reader để đọc..."));
    Serial.println(F("🔄 Auto-reconnect enabled"));
    mfrc522.PCD_DumpVersionToSerial();
  }
  
  Serial.println(F(" Sơ đồ kết nối YoloUNO:"));
  Serial.println(F("   VCC  -> 3.3V"));
  Serial.println(F("   GND  -> GND"));
  Serial.println(F("   SS   -> D10 (GPIO 21)"));
  Serial.println(F("   SCK  -> D13 (GPIO 48)"));
  Serial.println(F("   MOSI -> D11 (GPIO 38)"));
  Serial.println(F("   MISO -> D12 (GPIO 47)"));
  Serial.println(F("   RST  -> D9  (GPIO 18)"));
  Serial.println();
}

// Hàm kiểm tra kết nối MFRC522 với detailed reporting
bool testRFIDConnection() {
  // Thử đọc version register
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  
  if (version == 0x00 || version == 0xFF) {
    Serial.printf("   ❌ Invalid version: 0x%02X\n", version);
    return false;
  } else {
    Serial.printf("   ✅ Valid version: 0x%02X\n", version);
    return true;
  }
}

// Hàm in mã thẻ dạng HEX
void printCardID() {
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      Serial.print(F("0"));
    }
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) {
      Serial.print(F(" "));
    }
  }
}

// Hàm hiển thị thông tin chi tiết của thẻ
void printCardDetails() {
  // Hiển thị loại thẻ
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.print(F("Loại thẻ: "));
  Serial.println(mfrc522.PICC_GetTypeName(piccType));
  
  // Hiển thị UID dạng decimal
  Serial.print(F("Mã thẻ (Dec): "));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i]);
    if (i < mfrc522.uid.size - 1) {
      Serial.print(F(" "));
    }
  }
  Serial.println();
  
  // Hiển thị UID dạng chuỗi liên tục
  Serial.print(F("Mã thẻ (String): "));
  String cardID = getCardIDString();
  Serial.println(cardID);
}

// Hàm lấy mã thẻ dạng chuỗi
String getCardIDString() {
  String cardID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      cardID += "0";
    }
    cardID += String(mfrc522.uid.uidByte[i], HEX);
  }
  cardID.toUpperCase();
  return cardID;
}

// Hàm kiểm tra RFID có hoạt động không
bool isRFIDWorking() {
  // Thử đọc version register
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  return (version != 0x00 && version != 0xFF);
}

// Hàm xử lý lỗi RFID với detailed logging
void handleRFIDError() {
  Serial.println(F("🔧 Performing RFID reset and reinitialize..."));
  
  // Reset MFRC522
  digitalWrite(RFID_RST_PIN, LOW);
  Serial.println(F("   - RST pin LOW"));
  delay(100);
  digitalWrite(RFID_RST_PIN, HIGH);
  Serial.println(F("   - RST pin HIGH"));
  delay(100);
  
  // Reinitialize
  mfrc522.PCD_Init();
  Serial.println(F("   - PCD_Init() completed"));
  delay(200);
}

// Hàm tự động kết nối lại RFID (retry vô hạn)
bool reconnectRFID() {
  Serial.println(F("==RFID CONNECTION LOST! Starting unlimited reconnection..."));
  Serial.println(F("Press any key to cancel or wait for reconnection..."));
  
  int retryCount = 0;
  
  while (true) { // Retry vô hạn
    retryCount++;
    Serial.printf("🔄 Reconnection attempt #%d...\n", retryCount);
    
    // Reset và khởi tạo lại
    handleRFIDError();
    
    // Test kết nối
    if (testRFIDConnection()) {
      Serial.println(F("==RFID RECONNECTED SUCCESSFULLY!=="));
      return true;
    }
    
    // Hiển thị status và đợi
    Serial.printf("==Attempt #%d failed. Retrying in 2 seconds...\n", retryCount);
    
    // Đợi 2 giây trước khi retry (có thể check Serial input để cancel)
    for (int i = 0; i < 20; i++) { // 20 x 100ms = 2s
      delay(100);
      
      // Có thể thêm logic để break nếu user input (optional)
      // if (Serial.available()) break;
    }
    
    // Hiển thị progress mỗi 10 lần thử
    if (retryCount % 10 == 0) {
      Serial.printf(" Still trying... (%d attempts completed)\n", retryCount);
      Serial.println(F("Tip: Check physical connections and power supply"));
    }
  }
  
  // Never reached due to infinite loop
  return false;
}

void rfidTask(void *parameter) {
  Serial.println("RFID Task started");
  
  // Đợi một chút để đảm bảo hệ thống đã khởi tạo xong
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  
  // Khởi tạo RFID
  setupRFID();
  
  bool rfidConnected = isRFIDWorking(); // Check initial status
  Serial.printf("Initial RFID status: %s\n", rfidConnected ? "CONNECTED" : "DISCONNECTED");
  
  while (true) {
    // 🚨 REAL-TIME CONNECTION CHECK - kiểm tra mỗi loop
    bool currentStatus = isRFIDWorking();
    
    if (currentStatus != rfidConnected) {
      // Status changed - report immediately
      if (currentStatus) {
        Serial.println(F("RFID CONNECTION RESTORED!"));
      } else {
        Serial.println(F("RFID CONNECTION LOST DETECTED!"));
      }
      rfidConnected = currentStatus;
    }
    
    // Nếu RFID bị disconnect, bắt đầu reconnection ngay lập tức
    if (!rfidConnected) {
      Serial.println(F("Starting immediate reconnection process..."));
      if (reconnectRFID()) {
        rfidConnected = true;
        // Tiếp tục với card scanning
      } else {
        // Never reached due to infinite retry
        continue;
      }
    }
    
    // Kiểm tra xem có thẻ mới không
    if (!mfrc522.PICC_IsNewCardPresent()) {
      vTaskDelay(50 / portTICK_PERIOD_MS); // Giảm delay để responsive hơn
      continue;
    }

    // Chọn một trong các thẻ
    if (!mfrc522.PICC_ReadCardSerial()) {
      // ⚠️ IMMEDIATE ERROR DETECTION
      Serial.println(F("Card read failed! Checking connection..."));
      if (!isRFIDWorking()) {
        Serial.println(F("RFID connection lost during card read!"));
        rfidConnected = false;
        continue; // Sẽ trigger reconnection ở loop tiếp theo
      }
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }

    // 🎉 SUCCESSFUL CARD READ
    Serial.println(F("✅ Card read successful!"));
    
    // Hiển thị UID của thẻ
    Serial.print(F("🔍 Mã thẻ: "));
    printCardID();
    Serial.println();
    
    // Hiển thị thông tin chi tiết của thẻ
    printCardDetails();
    
    // Lấy mã thẻ dưới dạng chuỗi
    String cardID = getCardIDString();
    
    // Gửi qua MQTT (giữ nguyên chức năng cũ)
    sendRfidData(cardID);
    
    // 🆕 XỬ LÝ ĐIỂM DANH VỚI GOOGLE SHEETS
    Serial.println(F("Processing attendance..."));
    processRFIDForAttendance(cardID);
    
    Serial.println(F("========================================"));
    
    // Dừng PICC
    mfrc522.PICC_HaltA();
    
    // Dừng mã hóa trên PCD
    mfrc522.PCD_StopCrypto1();
    
    // Đợi 3 giây trước khi đọc thẻ tiếp theo
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
} 