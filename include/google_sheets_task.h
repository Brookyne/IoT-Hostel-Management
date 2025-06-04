#ifndef GOOGLE_SHEETS_TASK_H
#define GOOGLE_SHEETS_TASK_H

#include <Arduino.h>
#include <ArduinoHttpClient.h>

// Attendance data structure
struct AttendanceData {
  String cardId;
  String name;
  String status;  // "IN" hoặc "OUT"
  String date;
  String time;
};

// Task function
void googleSheetsTask(void *parameter);

// Setup function
void setupGoogleSheets();

// Main attendance processing function
void processRFIDForAttendance(const String &cardId);

// Utility functions
String getCurrentDateTime();
String getUserNameFromCard(const String &cardId);
bool isCardRegistered(const String &cardId);
String determineAttendanceStatus(const String &cardId);
void updateLastAttendanceStatus(const String &cardId, const String &status);

// Google Sheets communication
bool sendAttendanceToGoogleSheets(const AttendanceData &data);

// Database sync functions
bool syncDatabaseFromGoogleSheets();
bool tryGetMethod();
bool tryPostMethod();
bool parseDatabaseResponse(const String &response);
bool followRedirectForJSON(const String &redirectUrl);
bool parseJSONResponse(const String &response);
void printCurrentDatabase();
unsigned long getLastSyncTime();
bool loadUserDatabaseFromSheets();

#endif 