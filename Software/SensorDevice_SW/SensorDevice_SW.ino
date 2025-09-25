#include "Camera.h"
#include "SensorIO_withExtLibs.h"
#include "PowerManager.h"
#include "MQTT.h"
#include "TimeManager.h"
#include <Preferences.h>
#include "mbedtls/base64.h"
#include "driver/rtc_io.h"
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "Webpages.h"
#include "NetworkManager.h"

// === Main Debug Switch ===
#define MAIN_DEBUG 1 // set to 1 to enable detailed logs from this file, 0 to disable

#if MAIN_DEBUG == 1
  #define DEBUG_MAIN_PRINT(x) Serial.print(x)
  #define DEBUG_MAIN_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_MAIN_PRINT(x)
  #define DEBUG_MAIN_PRINTLN(x)
#endif

// --- Pin for entering Setup Mode ---
#define SETUP_BUTTON_PIN 2 // IO2 pin is used to enter setup mode

const int FLASH_PIN = 4;
const int LEDC_CHANNEL = 1; // LEDC Channel
const int LEDC_FREQ = 5000;   // PWM Freq
const int LEDC_RESOLUTION = 8; // 8bit Res

// --- rtos handles and queue ---
TaskHandle_t cameraTaskHandle = NULL;
TaskHandle_t backgroundTaskHandle = NULL;
QueueHandle_t commandQueue;

// --- command enum for rtos queue ---
enum Command { CMD_SEND_SENSORS, CMD_SEND_ALL };

// --- Forward declarations for callbacks and tasks ---
void sendSensorData();
void sendAllData();
void performOnDemandSensorRead();
void backgroundTask(void *parameter);

// --- Global Variables & Objects ---
RTC_DATA_ATTR int bootCount = 0; // a counter that survives deep sleep
int   battery_level    = 1; // holds the battery level percentage
float ambient_temp     = 2.0;  // holds ambient temperature
float ambient_humidity = 3.0; // holds ambient humidity
float light_intensity  = 4.0;  // holds light intensity
float soil_temp        = 5.0; // holds soil temperature
float soil_moisture    = 60.0; // holds soil moisture
float soil_ec          = 7.0; // holds soil electrical conductivity
String ccu_address     = "";   // holds the ccu address

// create async server and websocket objects
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// create custom class objects
Preferences preferences; // create the preferences object for the flag
NetworkManager networkManager(&preferences);
TimeManager timeManager(MAIN_DEBUG); // create the time manager object
PowerManager powerManager(MAIN_DEBUG, &preferences); // create the power manager object
Camera camera; // create the camera object
SensorIO sensors( &battery_level, &ambient_temp, &ambient_humidity, &light_intensity, &soil_temp, &soil_moisture, &soil_ec, MAIN_DEBUG );
MQTTClient mqtt( &ccu_address, &powerManager, MAIN_DEBUG );

// --- Global flag for current mode ---
bool isSetupMode = false; // flag to determine the current operating mode
const unsigned long SETUP_MODE_TIMEOUT = 10 * 60 * 1000; // 10 minutes timeout for setup mode

// --- Wakeup Reason Handler ---
void handleWakeupReason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : // in case of waking up from button press
      DEBUG_MAIN_PRINTLN("[INFO] Wakeup caused by external signal using RTC_IO");
      preferences.putBool("setup_mode", true); // set the flag to enter setup mode
      DEBUG_MAIN_PRINTLN("[ACTION] Rebooting into setup mode...");
      ESP.restart(); // restart to apply the mode change
      break;
    case ESP_SLEEP_WAKEUP_TIMER : // in case of waking up from timer
      DEBUG_MAIN_PRINTLN("[INFO] Wakeup caused by timer");
      break;
    default : // in other cases
      DEBUG_MAIN_PRINTLN("[INFO] Wakeup was not caused by deep sleep");
      break;
  }
}

// --- Action Functions (called by dashboard or MQTT) ---
void sendSensorData() {
    if (!WiFi.isConnected()) {
        DEBUG_MAIN_PRINTLN("[WARN] Not connected to WiFi, cannot send data.");
        return;
    }
    delay(250); // for stabilize
    sensors.readAllSensors(0);
    JsonDocument dataDoc;
    dataDoc["bat_level"] = battery_level;
    dataDoc["amb_temp"] = ambient_temp;
    dataDoc["amb_humi"] = ambient_humidity;
    dataDoc["amb_light"] = light_intensity;
    dataDoc["soil_temp"] = soil_temp;
    dataDoc["soil_humi"] = soil_moisture;
    dataDoc["soil_ec"] = soil_ec;
    String output;
    serializeJson(dataDoc, output);
    mqtt.loop();
    mqtt.publishData(output);
}

void sendCameraData() {
    if (!WiFi.isConnected()) return; // exit if not connected
    delay(250); // for stabilize
    DEBUG_MAIN_PRINTLN("[ACTION] Capturing high quality frame to send...");
    camera_fb_t* fb = camera.captureFrameForAnalyze(); // capture a high quality frame
    if (fb) { // if frame capture was successful
        DEBUG_MAIN_PRINTLN("[DEBUG] Frame captured successfully.");
        size_t output_len;
        mbedtls_base64_encode(NULL, 0, &output_len, fb->buf, fb->len);
        unsigned char *base64_buf = (unsigned char *)malloc(output_len + 1);
        if (base64_buf == NULL) {
            DEBUG_MAIN_PRINTLN("[ERROR] Failed to allocate memory for Base64 buffer!");
            camera.releaseFrameForAnalyze(fb);
            return;
        }
        if(mbedtls_base64_encode(base64_buf, output_len + 1, &output_len, fb->buf, fb->len) != 0) {
            DEBUG_MAIN_PRINTLN("[ERROR] Base64 encoding failed!");
            free(base64_buf);
            camera.releaseFrameForAnalyze(fb);
            return;
        }
        base64_buf[output_len] = '\0'; // ensure null termination
        JsonDocument dataDoc;
        dataDoc["plant_img"] = (char*)base64_buf;
        String output;
        serializeJson(dataDoc, output);
        mqtt.loop();
        mqtt.publishData(output); // publish the data
        free(base64_buf);
    } else {
        DEBUG_MAIN_PRINTLN("[ERROR] Failed to capture high quality frame for sending.");
    }
    camera.releaseFrameForAnalyze(fb); // release the frame buffer
}

void sendAllData() {
    sendSensorData();
    delay(100); // Small delay between sends
    sendCameraData();
}

void handleDataRequest() {
    DEBUG_MAIN_PRINTLN("[MQTT] Data request received from CCU.");
    sendSensorData(); // send sensor data in response
}

void handleConfig(JsonDocument& doc, PowerManager& pm) {
    DEBUG_MAIN_PRINTLN("[MQTT] Config received from CCU.");
    if (!doc["pwr_mode"].isNull()) { // if the json contains 'pwr_mode'
        const char* pwr_mode_str = doc["pwr_mode"]; // get the value as a string
        pm.setMode(pwr_mode_str[0]); // use the first character to set the mode
    }
    if (!doc["nht_mode"].isNull()) { // if the json contains 'nht_mode'
        bool nht_mode = doc["nht_mode"].as<bool>(); // get the boolean value
        pm.setNightMode(nht_mode); // set the new night mode
    }
}

void performOnDemandSensorRead() {
    DEBUG_MAIN_PRINTLN("[ACTION] On-demand sensor read initiated.");
    delay(100);
    sensors.begin(); // Initializes I2C and sensors
    delay(100);
    sensors.readAllSensors(0); // Read all sensor values in normal mode
    delay(100);
    sensors.endI2C(); // De-initialize I2C to free up pin 2
    DEBUG_MAIN_PRINTLN("[ACTION] On-demand sensor read complete. I2C released.");
}

// this function replaces placeholders in the html template
String processor(const String& var){
  if(var == "DASHBOARD_TITLE"){ return "GreenEye 센서단말<br>[" + networkManager.getHostname() + "]"; }
  if(var == "CURRENT_SSID"){
    preferences.begin("wifi-creds", true);
    String ssid = preferences.getString("ssid", "N/A");
    preferences.end();
    return ssid;
  }
  if(var == "CURRENT_CCU_ADDRESS"){
    preferences.begin("wifi-creds", true);
    String ccu = preferences.getString("ccu_address", "Not Set");
    preferences.end();
    return ccu;
  }
  return String();
}

// --- RTOS Task Functions ---
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) { 
    switch (type) { 
        case WS_EVT_CONNECT: 
            Serial.printf("[WebSocket] Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str()); 
            // if this is the first client, resume the camera task 
            if (ws.count() == 1 && cameraTaskHandle != NULL) { 
                Serial.println("[ACTION] First WebSocket client connected, resuming camera task."); 
                vTaskResume(cameraTaskHandle); 
            } 
            break; 
        case WS_EVT_DISCONNECT: 
            Serial.printf("[WebSocket] Client #%u disconnected\n", client->id()); 
            // if the last client disconnected, suspend the camera task to save resources 
            if (ws.count() == 0 && cameraTaskHandle != NULL) { 
                Serial.println("[ACTION] Last WebSocket client disconnected, suspending camera task."); 
                vTaskSuspend(cameraTaskHandle); 
            } 
            break; 
        case WS_EVT_DATA: 
            // data event 
        break; 
        case WS_EVT_PONG: 
        case WS_EVT_ERROR: 
            // other events 
        break; 
    } 
} 

void cameraStreamTask(void *pvParameters) {
  while (1) {
    if (ws.count() > 0) {
      camera_fb_t *fb = camera.captureFrameForStream();
      if (fb) {
        ws.binaryAll(fb->buf, fb->len);
        camera.releaseFrameForStream(fb);
      }
    } else { vTaskDelay(pdMS_TO_TICKS(100)); }
    vTaskDelay(pdMS_TO_TICKS(100)); // ~10 fps
  }
}

void backgroundTask(void *parameter) {
    DEBUG_MAIN_PRINTLN("Background Task started on core 0.");
    static unsigned long buttonPressStartTime = 0;
    for (;;) {
        mqtt.loop();
        if (digitalRead(SETUP_BUTTON_PIN) == LOW) {
            if (buttonPressStartTime == 0) {
                buttonPressStartTime = millis();
            } else if (millis() - buttonPressStartTime > 3000) {
                DEBUG_MAIN_PRINTLN("[ACTION] Exiting Setup mode via button press. Restarting...");
                preferences.begin("device-state", false);
                preferences.putBool("setup_mode", false);
                preferences.end();
                ESP.restart();
            }
        } else {
            buttonPressStartTime = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void setup() {
    ledcSetup(LEDC_CHANNEL, LEDC_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(FLASH_PIN, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, 1);
    if(MAIN_DEBUG){
        Serial.begin(115200); // initialize serial communication
        delay(1000); // wait for serial monitor to open
    }
    if(!camera.begin()){ DEBUG_MAIN_PRINTLN("[ERROR] Failed to start camera"); } // initialize camera
    
    delay(2000); // wait for io pins to stabilize
    bootCount++; // increment the deep sleep wake-up counter
    DEBUG_MAIN_PRINT("[INFO] === Boot count: ");
    DEBUG_MAIN_PRINT(bootCount); DEBUG_MAIN_PRINTLN(" ===");

    preferences.begin("device-state", false); // initialize preferences for the mode flag
    powerManager.begin();
    handleWakeupReason(); // check why the device woke up
    bool enter_setup_flag = preferences.getBool("setup_mode", false); // check if the flag is set
    if (enter_setup_flag) {
        DEBUG_MAIN_PRINTLN("[DEBUG] SETUP MODE FLAG is HIGH.");
        isSetupMode = true; // enter setup mode
    }
    preferences.end();

        if (isSetupMode) {
        DEBUG_MAIN_PRINTLN("[MODE] SETUP MODE ACTIVATED.");
        ledcWrite(LEDC_CHANNEL, 0); delay(500); ledcWrite(LEDC_CHANNEL, 1); delay(500);
        ledcWrite(LEDC_CHANNEL, 0); delay(500); ledcWrite(LEDC_CHANNEL, 1); delay(500);
        ledcWrite(LEDC_CHANNEL, 0);

        networkManager.begin();
        sensors.begin();

        ws.onEvent(onWsEvent);
        server.addHandler(&ws);
        // setup all web server routes
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            DEBUG_MAIN_PRINTLN("[WebServer] Root ('/') page requested.");
            const char* content = WiFi.isConnected() ? DEVICE_STATUS_CONTENT : SETUP_FORM_CONTENT;
            // replace placeholders and send the page
            request->send_P(200, "text/html", DASHBOARD_MAIN_TEMPLATE, [content](const String& var) -> String {
                if (var == "PAGE_CONTENT") return FPSTR(content);
                return processor(var);
            });
        });
        server.on("/dashboard", HTTP_GET, [](AsyncWebServerRequest *request) {
            DEBUG_MAIN_PRINTLN("[WebServer] Dashboard ('/dashboard') page requested.");
            request->send_P(200, "text/html", DASHBOARD_MAIN_TEMPLATE, [](const String& var) -> String {
                if (var == "PAGE_CONTENT") return FPSTR(DASHBOARD_CONTENT);
                return processor(var);
            });
        });
        server.on("/api/sensors", HTTP_GET, [](AsyncWebServerRequest *request){
            DEBUG_MAIN_PRINTLN("[WebServer] API ('/api/sensors') data requested.");
            sensors.readAllSensors(0);
            
            JsonDocument dataDoc;
            dataDoc["bat_level"] = battery_level;
            dataDoc["pwr_mode"] = powerManager.getModeString();
            dataDoc["night_mode"] = powerManager.isNightModeEnabled() ? "ON" : "OFF";
            dataDoc["amb_temp"] = ambient_temp;
            dataDoc["amb_humi"] = ambient_humidity;
            dataDoc["amb_light"] = light_intensity;
            dataDoc["soil_temp"] = soil_temp;
            dataDoc["soil_humi"] = soil_moisture;
            dataDoc["soil_ec"] = soil_ec;
            
            String output;
     
            serializeJson(dataDoc, output);
            request->send(200, "application/json", output);
        });
        server.on("/camera", HTTP_GET, [](AsyncWebServerRequest *request){
            DEBUG_MAIN_PRINTLN("[WebServer] Camera ('/camera') page requested.");
            request->send_P(200, "text/html", WEBSOCKET_CAMERA_PAGE_HTML, processor);
        });
        server.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request){
            DEBUG_MAIN_PRINTLN("[WebServer] Debug ('/debug') page requested.");
            request->send_P(200, "text/html", DASHBOARD_MAIN_TEMPLATE, [](const String& var) -> String {
                if (var == "PAGE_CONTENT") return FPSTR(DEBUG_PAGE_CONTENT);
                return processor(var);
            });
        });
        // handle form submissions
        server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
            DEBUG_MAIN_PRINTLN("[WebServer] POST to '/save' received.");
            String getSsid = (request->hasParam("ssid", true)) ? request->getParam("ssid", true)->value():"";
            String getPw = (request->hasParam("password", true)) ? request->getParam("password", true)->value():"";
            String getCcuAddr = (request->hasParam("ccu_address", true)) ? request->getParam("ccu_address", true)->value():"";
            DEBUG_MAIN_PRINTLN("[INFO] Recieved Network datas");
            DEBUG_MAIN_PRINT("     SSID : "); DEBUG_MAIN_PRINTLN(getSsid);
            DEBUG_MAIN_PRINT("     PW : "); DEBUG_MAIN_PRINTLN(getPw);
            DEBUG_MAIN_PRINT("     CCU ADDR : "); DEBUG_MAIN_PRINTLN(getCcuAddr);

            preferences.begin("wifi-creds", false);
            preferences.putString("ssid", getSsid);
            preferences.putString("password", getPw);
            preferences.putString("ccu_address", getCcuAddr);
            preferences.end();
            request->send_P(200, "text/html", SUCCESS_PAGE_HTML);
            delay(1000); ESP.restart();
        });
        server.on("/forget", HTTP_POST, [](AsyncWebServerRequest *request){
            DEBUG_MAIN_PRINTLN("[WebServer] POST to '/forget' received.");
            preferences.begin("wifi-creds", false);
            preferences.remove("ssid");
            preferences.remove("password");
            preferences.remove("ccu_address");
            preferences.end();
            DEBUG_MAIN_PRINTLN("[INFO] Clear preference!");
            request->send_P(200, "text/html", FORGET_SUCCESS_PAGE_HTML);
            delay(1000); ESP.restart();
        });
        server.on("/save_ccu", HTTP_POST, [](AsyncWebServerRequest *request){
            DEBUG_MAIN_PRINTLN("[WebServer] POST to '/save_ccu' received.");
            preferences.begin("wifi-creds", false);
            if(request->hasParam("ccu_address", true)) preferences.putString("ccu_address", request->getParam("ccu_address", true)->value());
            preferences.end();
            request->send_P(200, "text/html", SUCCESS_PAGE_HTML);
            delay(1000);
            ESP.restart();
        });
        server.on("/set_power_mode", HTTP_POST, [](AsyncWebServerRequest *request){
            DEBUG_MAIN_PRINTLN("[WebServer] POST to '/set_power_mode' received.");
            if(request->hasParam("pwr_mode", true)) {
                String mode = request->getParam("pwr_mode", true)->value();
                powerManager.setMode(mode[0]);
            }
            request->redirect("/debug");
        });
        server.on("/set_night_mode", HTTP_POST, [](AsyncWebServerRequest *request){
            DEBUG_MAIN_PRINTLN("[WebServer] POST to '/set_night_mode' received.");
            if(request->hasParam("nht_mode", true)) {
                bool nightMode = request->getParam("nht_mode", true)->value().toInt() == 1;
                powerManager.setNightMode(nightMode);
            }
            request->redirect("/debug");
        });

        server.on("/send_sensor", HTTP_POST, [](AsyncWebServerRequest *request){
            DEBUG_MAIN_PRINTLN("[WebServer] POST to '/send_sensor' received.");
            sendSensorData(); // call the function to send sensor data
            request->redirect("/debug"); // redirect back to the debug page
        });

        server.on("/send_all", HTTP_POST, [](AsyncWebServerRequest *request){
            DEBUG_MAIN_PRINTLN("[WebServer] POST to '/send_all' received.");
            sendAllData(); // call the function to send all data (sensor + camera)
            request->redirect("/debug"); // redirect back to the debug page
        });

        server.begin(); // start server

        // create rtos tasks for setup mode
        xTaskCreatePinnedToCore(backgroundTask, "Background Task", 4096, NULL, 1, &backgroundTaskHandle, 0);
        xTaskCreatePinnedToCore(cameraStreamTask, "Camera Stream Task", 4096, NULL, 2, &cameraTaskHandle, 0);

        if (cameraTaskHandle != NULL) { 
            vTaskSuspend(cameraTaskHandle); 
        } 
    } else {
        DEBUG_MAIN_PRINTLN("[MODE] NORMAL MODE ACTIVATED."); 
        ledcWrite(LEDC_CHANNEL, 0); delay(500); ledcWrite(LEDC_CHANNEL, 1); delay(500);
        ledcWrite(LEDC_CHANNEL, 0);
        networkManager.begin();
        sensors.begin();
    }

    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_MAIN_PRINTLN("[INFO] WiFi connected! setup logic start!");
        mqtt.onDataRequest(handleDataRequest);
        mqtt.onConfig(handleConfig);
        mqtt.begin();
        timeManager.begin();
        timeManager.updateTime();
    } else {
        if (!isSetupMode) {
             DEBUG_MAIN_PRINTLN("[ACTION] No WiFi in Normal Mode, entering setup mode on next boot.");
             preferences.begin("device-state", false);
             preferences.putBool("setup_mode", true);
             preferences.end();
             ESP.restart();
        }
    }

    // load ccu address from preferences into the global variable
    preferences.begin("wifi-creds", true); // open in read-only mode
    ccu_address = preferences.getString("ccu_address", "");
    preferences.end();

    if (MAIN_DEBUG && !ccu_address.isEmpty()) {
        DEBUG_MAIN_PRINT("[INFO] Loaded CCU Address: ");
        DEBUG_MAIN_PRINTLN(ccu_address);
    }
}

void loop() {
  if (isSetupMode) {
    // the loop is empty in setup mode because rtos tasks are running
    //vTaskDelay(pdMS_TO_TICKS(10));
  } else {
    // --- Normal Mode: Sense, Transmit, Sleep ---
    ledcWrite(LEDC_CHANNEL, 0);
    if (powerManager.isNightModeEnabled() && timeManager.isNightTime() && powerManager.getCurrentMode() != DEBUGGING) {
        DEBUG_MAIN_PRINTLN("[INFO] Night mode active. Woke up to check for MQTT updates.");
        if (WiFi.status() == WL_CONNECTED && mqtt.loop()) {
            DEBUG_MAIN_PRINTLN("[INFO] MQTT check complete.");
        } else {
            DEBUG_MAIN_PRINTLN("[WARN] Could not connect to WiFi/MQTT during night check.");
        }
        unsigned long sleep_duration_seconds = min(10UL * 60, timeManager.getSecondsUntil6AM());
        if (sleep_duration_seconds > 10) { 
            powerManager.enterDeepSleep(sleep_duration_seconds);
        } else {
            DEBUG_MAIN_PRINTLN("[INFO] Night mode period ending. Proceeding to normal operation.");
        }
    } 
    else {
        if (WiFi.status() == WL_CONNECTED) { 
          DEBUG_MAIN_PRINTLN("[INFO] WiFi Connected.");
          if (mqtt.loop()) { 
              DEBUG_MAIN_PRINTLN("[INFO] MQTT broker connected.");
              sendSensorData();
              if (powerManager.shouldSendCameraData(bootCount)) {
                  sendCameraData();
              }
              delay(2000);
              sensors.endI2C();
              powerManager.enterDeepSleep(powerManager.getSenseInterval());
          } else {
              DEBUG_MAIN_PRINTLN("\n[ERROR] Failed to connect to MQTT broker, entering setup mode on next boot.");
              preferences.putBool("setup_mode", true);
              ESP.restart();
          }
        } else {
          DEBUG_MAIN_PRINTLN("\n[ERROR] Failed to connect to WiFi, entering setup mode on next boot.");
          preferences.putBool("setup_mode", true);
          ESP.restart();
        }
    }
  }
}