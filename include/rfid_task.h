#ifndef RFID_TASK_H
#define RFID_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <MFRC522.h>
#include <SPI.h>

// Định nghĩa chân kết nối RFID theo pin out YoloUNO
#define RFID_SS_PIN    21   // D10
#define RFID_RST_PIN   18    // D9
#define RFID_MOSI_PIN  38   // D11
#define RFID_MISO_PIN  47   // D12  
#define RFID_SCK_PIN   48   // D13

// External MFRC522 instance
extern MFRC522 mfrc522;

// Function declarations
void rfidTask(void *parameter);
void setupRFID();
bool testRFIDConnection();
void printCardID();
void printCardDetails();
String getCardIDString();
bool reconnectRFID();
bool isRFIDWorking();
void handleRFIDError();

#endif /* RFID_TASK_H */ 