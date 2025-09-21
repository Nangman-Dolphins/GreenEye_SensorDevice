#include "Camera.h"
#include "Dashboard.h"
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
TaskHandle_t webServerTaskHandle = NULL;
TaskHandle_t backgroundTaskHandle = NULL;
QueueHandle_t commandQueue;

// --- command enum for rtos queue ---
enum Command { CMD_SEND_SENSORS, CMD_SEND_ALL };

// --- Forward declarations for callbacks and tasks ---
void sendSensorData();
void sendAllData();
void performOnDemandSensorRead();
void webServerTask(void *parameter);
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

TimeManager timeManager(MAIN_DEBUG); // create the time manager object
PowerManager powerManager(MAIN_DEBUG); // create the power manager object
Camera camera; // create the camera object
SensorIO sensors(              // create the sensor io class
    &battery_level,
    &ambient_temp,
    &ambient_humidity,
    &light_intensity,
    &soil_temp,
    &soil_moisture,
    &soil_ec,
    MAIN_DEBUG
);
MQTTClient mqtt(                // create the mqtt client object
  &ccu_address,
  &powerManager,
  MAIN_DEBUG
);
Dashboard dashboard(
    &battery_level,
    &ambient_temp,
    &ambient_humidity,
    &light_intensity,
    &soil_temp,
    &soil_moisture,
    &soil_ec,
    &camera,
    &ccu_address,
    &powerManager,              // Pass power manager instance
    &sendSensorData,            // Pass sensor data sending function
    &sendAllData,               // Pass all data sending function
    &performOnDemandSensorRead,
    "defaultPW",                // Access Point Password
    MAIN_DEBUG
);
Preferences preferences;       // create the preferences object for the flag

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
    // in setup mode, send command to the background task queue
    if (isSetupMode) {
        Command cmd = CMD_SEND_SENSORS;
        xQueueSend(commandQueue, &cmd, portMAX_DELAY);
    } else {
        // in normal mode, execute directly
        if (!dashboard.isConnected()) {
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
}

void sendCameraData() {
    if (!dashboard.isConnected()) return; // exit if not connected

    delay(250); // for stabilize

    DEBUG_MAIN_PRINTLN("[ACTION] Capturing high quality frame to send...");
    camera_fb_t* fb = camera.captureFrameForAnalyze(); // capture a high quality frame

    if (fb) { // if frame capture was successful
        DEBUG_MAIN_PRINTLN("[DEBUG] Frame captured successfully.");
        size_t output_len;
        // calculate the required buffer size for base64
        mbedtls_base64_encode(NULL, 0, &output_len, fb->buf, fb->len);
        unsigned char *base64_buf = (unsigned char *)malloc(output_len + 1); // +1 for null terminator
        if (base64_buf == NULL) {
            DEBUG_MAIN_PRINTLN("[ERROR] Failed to allocate memory for Base64 buffer!");
            camera.releaseFrameForAnalyze(fb);
            return;
        }
        // perform the actual encoding
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
    // in setup mode, send command to the background task
    if (isSetupMode) {
        Command cmd = CMD_SEND_ALL;
        xQueueSend(commandQueue, &cmd, portMAX_DELAY);
    } else {
        // in normal mode, execute directly
        sendSensorData();
        delay(100); // Small delay between sends
        sendCameraData();
    }
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

void print_memory_status(const char* step) {
    DEBUG_MAIN_PRINT("--- Memory Status after ");
    DEBUG_MAIN_PRINT(step);
    DEBUG_MAIN_PRINTLN(" ---");
    DEBUG_MAIN_PRINT("Free Heap: ");
    DEBUG_MAIN_PRINT(ESP.getFreeHeap());
    DEBUG_MAIN_PRINTLN(" bytes");
    if (psramFound()) {
        DEBUG_MAIN_PRINT("Free PSRAM: ");
        DEBUG_MAIN_PRINT(ESP.getFreePsram());
        DEBUG_MAIN_PRINTLN(" bytes");
    } else {
        DEBUG_MAIN_PRINTLN("No PSRAM found!");
    }
    DEBUG_MAIN_PRINTLN("--------------------------------------");
}

void performOnDemandSensorRead() {
    // this function is called by the background task in setup mode
    DEBUG_MAIN_PRINTLN("[ACTION] On-demand sensor read initiated.");
    delay(100);
    sensors.begin(); // Initializes I2C and sensors
    delay(100);
    sensors.readAllSensors(0); // Read all sensor values in normal mode
    delay(100);
    sensors.endI2C(); // De-initialize I2C to free up pin 2
    DEBUG_MAIN_PRINTLN("[ACTION] On-demand sensor read complete. I2C released.");
}

// --- RTOS Task Functions ---
void webServerTask(void *parameter) {
    DEBUG_MAIN_PRINTLN("WebServer Task started on core 1.");
    for (;;) {
        dashboard.loop(); // handle web server requests continuously
        vTaskDelay(pdMS_TO_TICKS(10)); // yield to other tasks
    }
}

void backgroundTask(void *parameter) {
    DEBUG_MAIN_PRINTLN("Background Task started on core 0.");
    Command receivedCmd;
    static unsigned long buttonPressStartTime = 0;
    
    for (;;) {
        // handle incoming mqtt messages
        mqtt.loop();

        // handle commands from the queue (e.g., from web dashboard)
        if (xQueueReceive(commandQueue, &receivedCmd, 0) == pdPASS) {
            if (receivedCmd == CMD_SEND_SENSORS) {
                DEBUG_MAIN_PRINTLN("[RTOS] CMD_SEND_SENSORS received.");
                performOnDemandSensorRead();
                // re-create json data for mqtt publishing
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
                mqtt.publishData(output);
            } else if (receivedCmd == CMD_SEND_ALL) {
                 DEBUG_MAIN_PRINTLN("[RTOS] CMD_SEND_ALL received.");
                 performOnDemandSensorRead();
                 // call the original functions to send data
                 sendSensorData();
                 delay(100);
                 sendCameraData();
            }
        }

        // handle button press to exit setup mode
        if (digitalRead(SETUP_BUTTON_PIN) == LOW) {
            if (buttonPressStartTime == 0) {
                buttonPressStartTime = millis();
            } else if (millis() - buttonPressStartTime > 3000) {
                DEBUG_MAIN_PRINTLN("[ACTION] Exiting Setup mode via button press. Restarting...");
                preferences.putBool("setup_mode", false);
                ESP.restart();
            }
        } else {
            buttonPressStartTime = 0;
        }

        // handle setup mode timeout
        if (millis() > SETUP_MODE_TIMEOUT) {
            DEBUG_MAIN_PRINTLN("[WARN] Setup mode timed out. Restarting...");
            preferences.putBool("setup_mode", false);
            ESP.restart();
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // main background loop delay
    }
}

void setup() {
    ledcSetup(LEDC_CHANNEL, LEDC_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(FLASH_PIN, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, 10);
    if(MAIN_DEBUG){
        Serial.begin(115200); // initialize serial communication
        delay(1000); // wait for serial monitor to open
    }

    if(!camera.begin()){ DEBUG_MAIN_PRINTLN("[ERROR] Failed to start camera"); } // initialize camera
    print_memory_status("After camera init");

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

    // initialize subsystems based on the selected mode
    if (isSetupMode) {
        DEBUG_MAIN_PRINTLN("[MODE] SETUP MODE ACTIVATED.");
        ledcWrite(LEDC_CHANNEL, 0); delay(500);
        ledcWrite(LEDC_CHANNEL, 10); delay(500);
        ledcWrite(LEDC_CHANNEL, 0); delay(500);
        ledcWrite(LEDC_CHANNEL, 10); delay(500);
        ledcWrite(LEDC_CHANNEL, 0); delay(500);
        dashboard.begin();
    } else {
        DEBUG_MAIN_PRINTLN("[MODE] NORMAL MODE ACTIVATED."); 
        ledcWrite(LEDC_CHANNEL, 0); delay(500);
        ledcWrite(LEDC_CHANNEL, 10);
        dashboard.beginWiFi(); // wifi only
        sensors.begin(); // initialize sensors
    }

    performOnDemandSensorRead(); // init sensing

    DEBUG_MAIN_PRINT("[ACTION] Connecting to WiFi");
    int connection_timeout = 30; // wait for ~15 seconds
    while (WiFi.status() != WL_CONNECTED && connection_timeout > 0) {
        delay(500);
        DEBUG_MAIN_PRINT(".");
        connection_timeout--;
    }
    DEBUG_MAIN_PRINTLN("Complete!");

    // Initialize MQTT and TimeManager only if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_MAIN_PRINTLN("\n[INFO] MQTT Initialize.");
        mqtt.onDataRequest(handleDataRequest); // register the data request callback
        mqtt.onConfig(handleConfig); // register the config callback
        mqtt.begin(); // initialize mqtt client
        
        // initialize time manager and sync time
        timeManager.begin();
        timeManager.updateTime();
    } else {
        DEBUG_MAIN_PRINTLN("\n[WARN] WiFi connection failed in setup.");
        if (!isSetupMode) {
             DEBUG_MAIN_PRINTLN("[ACTION] No WiFi in Normal Mode, entering setup mode on next boot.");
             preferences.putBool("setup_mode", true);
             ESP.restart();
        }
    }
    print_memory_status("After WiFi init");

    if(isSetupMode) {
        // create a queue for commands between tasks
        commandQueue = xQueueCreate(5, sizeof(Command));

        // create rtos tasks for setup mode
        // pin web server to core 1, background to core 0
        xTaskCreatePinnedToCore(
            webServerTask,
            "WebServer Task",
            4096, // stack size
            NULL,
            1,    // priority
            &webServerTaskHandle,
            1
        );
        xTaskCreatePinnedToCore(
            backgroundTask,
            "Background Task",
            4096, // stack size
            NULL,
            1,    // priority
            &backgroundTaskHandle,
            0
        );
    }
}

void loop() {
  if (isSetupMode) {
    // the loop is empty in setup mode because rtos tasks are running
    vTaskDelay(pdMS_TO_TICKS(1000));
  } else {
    // --- Normal Mode: Sense, Transmit, Sleep ---
    ledcWrite(LEDC_CHANNEL, 0);
    // check if should enter the periodic night sleep cycle
    if (powerManager.isNightModeEnabled() && timeManager.isNightTime() && powerManager.getCurrentMode() != DEBUGGING) {
        DEBUG_MAIN_PRINTLN("[INFO] Night mode active. Woke up to check for MQTT updates.");
        // try to connect and check MQTT messages
        if (WiFi.status() == WL_CONNECTED && mqtt.loop()) {
            DEBUG_MAIN_PRINTLN("[INFO] MQTT check complete.");
        } else {
            DEBUG_MAIN_PRINTLN("[WARN] Could not connect to WiFi/MQTT during night check.");
        }
        // calculate sleep time (max 10 mins, or until 6 AM) and go back to sleep
        unsigned long sleep_duration_seconds = min(10UL * 60, timeManager.getSecondsUntil6AM());
        if (sleep_duration_seconds > 10) { // a small threshold to prevent sleeping for a few seconds if it's already 6 AM
            powerManager.enterDeepSleep(sleep_duration_seconds);
        } else {
            DEBUG_MAIN_PRINTLN("[INFO] Night mode period ending. Proceeding to normal operation.");
        }
    } 
    // if not in the night cycle, execute normal daytime operation
    else {
        if (WiFi.status() == WL_CONNECTED) { // if wifi connected successfully
          DEBUG_MAIN_PRINTLN("[INFO] WiFi Connected.");
          if (mqtt.loop()) { // mqtt.loop() also handles incoming messages
              DEBUG_MAIN_PRINTLN("[INFO] MQTT broker connected.");
              // always send sensor data on wakeup in normal mode
              sendSensorData();
              // check if it's time to send camera data
              if (powerManager.shouldSendCameraData(bootCount)) {
                  sendCameraData();
              }
              
              delay(2000); // wait for data to be sent before sleeping
              sensors.endI2C();
              // enter deep sleep for the configured interval
              powerManager.enterDeepSleep(powerManager.getSenseInterval());
          } else {
              // MQTT connection failed after retries
              DEBUG_MAIN_PRINTLN("\n[ERROR] Failed to connect to MQTT broker, entering setup mode on next boot.");
              preferences.putBool("setup_mode", true);
              ESP.restart(); // Restart to switch to setup mode immediately
          }
        } else {
          DEBUG_MAIN_PRINTLN("\n[ERROR] Failed to connect to WiFi, entering setup mode on next boot.");
          preferences.putBool("setup_mode", true);
          ESP.restart(); // Restart to switch to setup mode immediately
        }
    }
  }
}