/**
 * ESP32 OTA Firmware Update with DHT20 Sensor
 * 
 * A program that connects to ThingsBoard,
 * reads DHT20 sensor data (temperature and humidity), 
 * and supports OTA firmware updates using RTOS.
 * 
 * Hardware: ESP32 board with DHT20 sensor
 */

 #define CONFIG_THINGSBOARD_ENABLE_DEBUG false
 #include <WiFi.h>
 #include <Arduino_MQTT_Client.h>
 #include <ThingsBoard.h>
 #include <OTA_Firmware_Update.h>
 #include <Shared_Attribute_Update.h>
 #include <Attribute_Request.h>
 #include <Espressif_Updater.h>
 #include "DHT20.h"
 #include "Wire.h"
 #include <UNIT_ACMEASURE.h>
 // Pin definitions
 #define LED_PIN 48
 #define SDA_PIN GPIO_NUM_11
 #define SCL_PIN GPIO_NUM_12
 
 // Blink parameters - simple LED blinker to indicate device is running
 #define BLINK_INTERVAL 1000  // LED blink interval (ms)
 unsigned long lastBlink = 0;
 bool ledState = false;
 
 // Firmware information
 constexpr char FIRMWARE_TITLE[] = "OTA test";
 constexpr char FIRMWARE_VERSION[] = "2.0";
 
 // Shared attributes configuration
 constexpr uint8_t MAX_ATTRIBUTES = 2U;
 constexpr std::array<const char*, MAX_ATTRIBUTES> SHARED_ATTRIBUTES = {
   "fw_version",
   "fw_title"
 };
 
 // OTA parameters
 constexpr uint8_t FIRMWARE_FAILURE_RETRIES = 20U;
 constexpr uint16_t FIRMWARE_PACKET_SIZE = 8192U;
 
 // Telemetry parameters
 constexpr int16_t TELEMETRY_SEND_INTERVAL = 10000U;  // 10 seconds
 
 // WiFi and ThingsBoard configuration
 constexpr char WIFI_SSID[] = "Anonymous";
 constexpr char WIFI_PASSWORD[] = "Nguyen2004";
 constexpr char TB_SERVER[] = "app.coreiot.io";
 constexpr char TB_TOKEN[] = "c4o60sz3dxr3ukyeugl7";
 constexpr uint16_t TB_PORT = 1883U;
 constexpr uint16_t MAX_MESSAGE_SIZE = 512U;
 constexpr uint32_t SERIAL_BAUD = 115200U;
 constexpr uint64_t REQUEST_TIMEOUT_MS = 10000U * 1000U;
 
 // Status flags
 bool shared_attributes_subscribed = false;
 bool current_firmware_sent = false;
 bool update_requested = false;
 bool shared_requested = false;
 
 // WiFi client
 WiFiClient wifiClient;
 Arduino_MQTT_Client mqttClient(wifiClient);
 
 // ThingsBoard APIs
 OTA_Firmware_Update<> ota;
 Shared_Attribute_Update<1U, MAX_ATTRIBUTES> shared_update;
 Attribute_Request<2U, MAX_ATTRIBUTES> attr_request;
 const std::array<IAPI_Implementation*, 3U> apis = {
   &shared_update,
   &attr_request,
   &ota
 };
 
 // Initialize ThingsBoard instance
 ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE, MAX_MESSAGE_SIZE, Default_Max_Stack_Size, apis);
 Espressif_Updater<> updater;
 
 // Create DHT20 instance
 DHT20 dht20;
 
 // Callback for processing shared attribute updates
 void processSharedAttributeUpdate(const JsonObjectConst &data) {
   Serial.println("Received shared attribute update:");
   const size_t jsonSize = Helper::Measure_Json(data);
   char buffer[jsonSize];
   serializeJson(data, buffer, jsonSize);
   Serial.println(buffer);
 }
 // Callback for received shared attribute values
 void processSharedAttributeRequest(const JsonObjectConst &data) {
   Serial.println("Received shared attribute values:");
   const size_t jsonSize = Helper::Measure_Json(data);
   char buffer[jsonSize];
   serializeJson(data, buffer, jsonSize);
   Serial.println(buffer);
 }
 
 // Timeout callback for attribute requests
 void requestTimeoutCallback() {
   Serial.println("Attribute request timed out. Check connection and attribute names.");
 }
 
 // OTA update callbacks
 void updateStartingCallback() {
   Serial.println("OTA update is starting...");
 }
 
 void updateProgressCallback(const size_t &current, const size_t &total) {
   static int lastPercent = 0;
   int percent = (current * 100) / total;
   if (percent != lastPercent) {
     Serial.printf("Download progress: %d%% (%zu/%zu bytes)\n", 
                  percent, current, total);
     lastPercent = percent;
   }
 }
 
 void updateFinishedCallback(const bool &success) {
   if (success) {
     Serial.println("Update successful! Rebooting...");
     delay(1000);
     ESP.restart();
   } else {
     Serial.println("Update failed!");
     // Reset flags to try again later
     update_requested = false;
   }
 }
 
 // Initialize WiFi connection
 void initWiFi() {
   Serial.println("Connecting to WiFi...");
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
   
   int attempts = 0;
   while (WiFi.status() != WL_CONNECTED && attempts < 20) {
     delay(500);
     Serial.print(".");
     attempts++;
   }
   
   if (WiFi.status() == WL_CONNECTED) {
     Serial.println("\nConnected to WiFi");
     Serial.print("IP Address: ");
     Serial.println(WiFi.localIP());
   } else {
     Serial.println("\nFailed to connect to WiFi. Will retry later.");
   }
 }
 
 // Check and reconnect WiFi if needed
 bool reconnectWiFi() {
   if (WiFi.status() == WL_CONNECTED) {
     return true;
   }
   
   Serial.println("WiFi connection lost. Reconnecting...");
   WiFi.disconnect();
   delay(1000);
   initWiFi();
   return (WiFi.status() == WL_CONNECTED);
 }
 
 // Initialize ThingsBoard connection and subscriptions
 bool initThingsBoard() {
   if (tb.connected()) {
     return true;
   }
 
   Serial.printf("Connecting to ThingsBoard at %s with token %s\n", TB_SERVER, TB_TOKEN);
   if (!tb.connect(TB_SERVER, TB_TOKEN, TB_PORT)) 
   {
     Serial.println("Failed to connect to ThingsBoard. Retrying in 5 seconds...");
     return false;
   }
   
   Serial.println("Connected to ThingsBoard!");
   
   // Send device metadata
   tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
   tb.sendAttributeData("deviceType", "Complete Environment Monitor");
   tb.sendAttributeData("firmwareVersion", FIRMWARE_VERSION);
   
   return true;
 }
 
 // Set up shared attributes
 bool setupSharedAttributes() {
   // Request shared attributes if not done already
   if (!shared_requested) {
     Serial.println("Requesting shared attributes...");
     const Attribute_Request_Callback<MAX_ATTRIBUTES> sharedCallback(
       &processSharedAttributeRequest, 
       REQUEST_TIMEOUT_MS, 
       &requestTimeoutCallback, 
       SHARED_ATTRIBUTES
     );
     shared_requested = attr_request.Shared_Attributes_Request(sharedCallback);
     if (!shared_requested) {
       Serial.println("Failed to request shared attributes");
       return false;
     }
   }
   
   // Subscribe to shared attribute updates if not done already
   if (!shared_attributes_subscribed) {
     Serial.println("Subscribing to shared attribute updates...");
     const Shared_Attribute_Callback<MAX_ATTRIBUTES> updateCallback(
       &processSharedAttributeUpdate, 
       SHARED_ATTRIBUTES
     );
     if (!shared_update.Shared_Attributes_Subscribe(updateCallback)) {
       Serial.println("Failed to subscribe to shared attribute updates");
       return false;
     } else {
       Serial.println("Subscribed to shared attribute updates");
       shared_attributes_subscribed = true;
     }
   }
   
   return true;
 }
 
 // Send firmware information and setup OTA updates
 bool setupOTAUpdates() {
   // Send current firmware info if not done already
   if (!current_firmware_sent) {
     current_firmware_sent = ota.Firmware_Send_Info(FIRMWARE_TITLE, FIRMWARE_VERSION);
     if (current_firmware_sent) {
       Serial.printf("Sent current firmware info: %s v%s\n", FIRMWARE_TITLE, FIRMWARE_VERSION);
     }
   }
   
   // Setup firmware update if not done already
   if (!update_requested) {
     Serial.println("Setting up OTA firmware update mechanism...");
     const OTA_Update_Callback callback(
       FIRMWARE_TITLE,
       FIRMWARE_VERSION,
       &updater,
       &updateFinishedCallback,
       &updateProgressCallback,
       &updateStartingCallback,
       FIRMWARE_FAILURE_RETRIES,
       FIRMWARE_PACKET_SIZE
     );
     
     update_requested = ota.Start_Firmware_Update(callback);
     if (update_requested) {
       Serial.println("OTA update mechanism initialized");
       Serial.println("Subscribing to firmware updates...");
       update_requested = ota.Subscribe_Firmware_Update(callback);
       if (update_requested) {
         Serial.println("Successfully subscribed to firmware updates");
       } else {
         Serial.println("Failed to subscribe to firmware updates");
         return false;
       }
     } else {
       Serial.println("Failed to initialize OTA update mechanism");
       return false;
     }
   }
   
   return true;
 }
 
 // Task functions for RTOS
 
 // WiFi reconnection task
 void wifi_reconnect_task(void *pvParameters) {
   while(1) {
     if (!reconnectWiFi()) {
       Serial.println("WiFi reconnection failed. Will retry...");
     } else {
       Serial.println("WiFi connection maintained.");
     }
     vTaskDelay(10000 / portTICK_PERIOD_MS);
   }
 }
 
 // ThingsBoard connection task
 void tb_connect_task(void *pvParameters) {
   while(1) {
     if (!tb.connected()) {
       initThingsBoard();
       setupSharedAttributes();
       setupOTAUpdates();
     }
     vTaskDelay(5000 / portTICK_PERIOD_MS);
   }
 }
 
 // LED blink task (simple device status indicator)
 void led_blink_task(void *pvParameters) {
   TickType_t xLastWakeTime = xTaskGetTickCount();
   
   while(1) {
     // Handle LED blinking
     if (millis() - lastBlink >= BLINK_INTERVAL) {
       ledState = !ledState;
       digitalWrite(LED_PIN, ledState);
       lastBlink = millis();
     }
     
     vTaskDelay(50 / portTICK_PERIOD_MS);
   }
 }
 
 // ThingsBoard message processing task
 void tb_loop_task(void *pvParameters) {
   while(1) {
     tb.loop();
     vTaskDelay(50 / portTICK_PERIOD_MS);
   }
 }
 
 // Sensor data collection and transmission task - TEMPERATURE AND HUMIDITY
 void sensor_task(void *pvParameters) {
   while(1) {
     // Read DHT20 sensor
     dht20.read();
     
     float temperature = dht20.getTemperature();
     float humidity = dht20.getHumidity();
     int powerCon = rand() % 99 + 1; 
     if (isnan(temperature) || isnan(humidity)) {
       Serial.println("Failed to read from DHT20 sensor!");
     } else {
       Serial.print("Temperature: ");
       Serial.print(temperature);
       Serial.print(" °C, Humidity: ");
       Serial.print(humidity);
       Serial.println(" %");
 
       // Send telemetry data - BOTH TEMPERATURE AND HUMIDITY
       tb.sendTelemetryData("temperature", temperature);
       tb.sendTelemetryData("humidity", humidity);
       tb.sendTelemetryData("powerConsumption", powerCon);
       
       
       // Send WiFi information as attributes
       tb.sendAttributeData("rssi", WiFi.RSSI());
       tb.sendAttributeData("channel", WiFi.channel());
       tb.sendAttributeData("bssid", WiFi.BSSIDstr().c_str());
       tb.sendAttributeData("localIp", WiFi.localIP().toString().c_str());
     }
     
     vTaskDelay(TELEMETRY_SEND_INTERVAL / portTICK_PERIOD_MS);
   }
 }
 
 // Utility function to calculate heat index
 
 
 void setup() {
   // Initialize serial communication
   Serial.begin(SERIAL_BAUD);
   delay(1000);
   
   // Print firmware information
   Serial.println("\n===========================");
   Serial.println("ESP32 Temperature & Humidity Monitor with OTA");
   Serial.printf("Firmware: %s v%s\n", FIRMWARE_TITLE, FIRMWARE_VERSION);
   Serial.println("===========================\n");
   
   // Initialize LED
   pinMode(LED_PIN, OUTPUT);
   digitalWrite(LED_PIN, LOW);
   
   // Initialize DHT20 sensor
   Wire.begin(SDA_PIN, SCL_PIN);
   dht20.begin();
   
   // Connect to WiFi
   initWiFi();
   
   // Create RTOS tasks
   xTaskCreate(wifi_reconnect_task, "WiFi_Reconnect", 4096, NULL, 1, NULL);
   xTaskCreate(tb_connect_task, "TB_Connect", 4096, NULL, 1, NULL);
   xTaskCreate(led_blink_task, "LED_Blink", 2048, NULL, 2, NULL);
   xTaskCreate(tb_loop_task, "TB_Loop", 4096, NULL, 3, NULL);
   xTaskCreate(sensor_task, "Sensor_Task", 4096, NULL, 2, NULL);
 }
 
 void loop() {
   // Main loop is empty because we're using RTOS tasks
   delay(1000);
 }