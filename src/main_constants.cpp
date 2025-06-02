#include "main_constants.h"

// EEPROM configuration
const int EEPROM_SIZE = 512;
const int SSID_ADDR = 0;
const int PASS_ADDR = 32;
const int ESP_MAX_SSID_LEN = 32;
const int ESP_MAX_PASS_LEN = 64; 

// NTP configuration
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600; // GMT +7 cho Việt Nam
const int   daylightOffset_sec = 0;    // Không có Daylight Saving ở Việt Nam