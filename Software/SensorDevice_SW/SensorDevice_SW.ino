// SensorDevice_SW.ino

#include "Camera.h"       // include the camera class
#include "Dashboard.h"    // include the dashboard class
#include "SensorsIO.h"    // include the sensor io class
#include "PowerManager.h" // include the power manager class
#include "MQTT.h"         // include the mqtt client class
#include <Preferences.h>  // include for using the setup mode flag
#include "mbedtls/base64.h" // include for base64 incoding

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
#define SETUP_BUTTON_PIN 0 // IO0 pin is used to enter setup mode

// --- Forward declarations for callbacks ---
void sendSensorData();
void sendAllData();

// --- Global Variables & Objects ---
RTC_DATA_ATTR int bootCount = 0; // a counter that survives deep sleep

int   battery_level    = 0; // holds the battery level percentage
float ambient_temp     = 0.0;  // holds ambient temperature
float ambient_humidity = 0.0; // holds ambient humidity
float light_intensity  = 0.0;  // holds light intensity
float soil_temp        = 0.0; // holds soil temperature
float soil_moisture    = 0.0; // holds soil moisture
float soil_ec          = 0.0; // holds soil electrical conductivity
String ccu_address     = "";   // holds the ccu address

PowerManager powerManager(MAIN_DEBUG); // create the power manager object
Camera camera;                 // create the camera object
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
      DEBUG_MAIN_PRINTLN("[INFO] Wakeup caused by timer"); break;
    default : // in other cases
      DEBUG_MAIN_PRINTLN("[INFO] Wakeup was not caused by deep sleep"); break;
  }
}

// --- MQTT Event Handler Functions ---

void sendSensorData() {
    sensors.readAllSensors(); // Make sure sensor data is fresh before sending
    mqtt.loop();
    if (!dashboard.isConnected()) { // check if connected to wifi
        DEBUG_MAIN_PRINTLN("[WARN] Not connected to WiFi, cannot send data."); return; // exit if not connected
    }
    JsonDocument dataDoc; // create a json document
    dataDoc["bat_level"] = battery_level; // add battery level to json
    dataDoc["amb_temp"] = ambient_temp; // add ambient temperature to json
    dataDoc["amb_humi"] = ambient_humidity; // add ambient humidity to json
    dataDoc["amb_light"] = light_intensity; // add light intensity to json
    dataDoc["soil_temp"] = soil_temp; // add soil temperature to json
    dataDoc["soil_humi"] = soil_moisture; // add soil moisture to json
    dataDoc["soil_ec"] = soil_ec; // add soil ec to json
    String output; // create a string to hold the json
    serializeJson(dataDoc, output); // convert json to string
    mqtt.publishData(output);       // publish the data
}

void sendCameraData() {
    if (!dashboard.isConnected()) return; // exit if not connected
    
    DEBUG_MAIN_PRINTLN("[ACTION] Capturing high quality frame to send...");
    camera_fb_t* fb = camera.captureFrameForAnalyze(); // capture a high quality frame

    if (fb) { // if frame capture was successful
        DEBUG_MAIN_PRINTLN("[DEBUG] Frame captured successfully.");
        size_t output_len; // to store the length of the encoded data

        mbedtls_base64_encode(NULL, 0, &output_len, fb->buf, fb->len);
        
        // allocate memory for the base64 buffer. +1 for null terminator.
        unsigned char *base64_buf = (unsigned char *)malloc(output_len);
        if (base64_buf == NULL) {
            DEBUG_MAIN_PRINTLN("[ERROR] Failed to allocate memory for Base64 buffer!");
            camera.releaseFrameForAnalyze(fb); // IMPORTANT: release frame buffer before returning
            return;
        }

        mbedtls_base64_encode(base64_buf, output_len, &output_len, fb->buf, fb->len);

        JsonDocument dataDoc;
        dataDoc["plant_img"] = (char*)base64_buf;
        
        String output;
        serializeJson(dataDoc, output);
        
        mqtt.loop();
        mqtt.publishData(output); // publish the data
        
        free(base64_buf);
        camera.releaseFrameForAnalyze(fb); // release the frame buffer
    } else {
        DEBUG_MAIN_PRINTLN("[ERROR] Failed to capture high quality frame for sending.");
    }
}

// ADDED: Wrapper function to send both sensor and camera data
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
    DEBUG_MAIN_PRINTLN("[MQTT] Config received from CCU."); if (!doc["pwr_mode"].isNull()) { // if the json contains 'pwr_mode'
        const char* pwr_mode_str = doc["pwr_mode"]; // get the value as a string
        pm.setMode(pwr_mode_str[0]); // use the first character to set the mode
    }
    if (!doc["nht_mode"].isNull()) { // if the json contains 'nht_mode'
        bool nht_mode = doc["nht_mode"].as<bool>(); // get the boolean value
        pm.setNightMode(nht_mode); // set the new night mode
    }
}

void setup() {
  Serial.begin(115200); // initialize serial communication
  delay(1000); // wait for serial monitor to open

  pinMode(SETUP_BUTTON_PIN, INPUT_PULLUP);

  DEBUG_MAIN_PRINTLN("[INFO] Waiting 2 seconds for IO0 pin to stabilize...");
  delay(2000); bootCount++; // increment the deep sleep wake-up counter
  DEBUG_MAIN_PRINT("[INFO] === Boot count: ");
  DEBUG_MAIN_PRINT(bootCount); DEBUG_MAIN_PRINTLN(" ===");

  preferences.begin("device-state", false); // initialize preferences for the mode flag
  powerManager.begin();
 
  handleWakeupReason(); // check why the device woke up

  bool enter_setup_flag = preferences.getBool("setup_mode", false); // check if the flag is set

  if (enter_setup_flag) { // if the flag was set before rebooting
    DEBUG_MAIN_PRINTLN("[DEBUG] SETUP MODE FLAG is HIGH."); isSetupMode = true; // enter setup mode
    //preferences.putBool("setup_mode", false); // clear the flag for the next boot
  }

  if (digitalRead(SETUP_BUTTON_PIN) == LOW) {
    DEBUG_MAIN_PRINTLN("[INFO] IO0 pin is LOW. Waiting for it to go HIGH to start Normal Mode..."); 
    while(digitalRead(SETUP_BUTTON_PIN) == LOW) {
      delay(100);
      DEBUG_MAIN_PRINT("."); 
    }
    DEBUG_MAIN_PRINTLN("\n[INFO] IO0 is now HIGH. Continuing boot."); 
  } 

  // initialize subsystems based on the selected mode
  if (isSetupMode) {
    DEBUG_MAIN_PRINTLN("[MODE] SETUP MODE ACTIVATED."); if(!camera.begin()){ DEBUG_MAIN_PRINTLN("[ERROR] Failed to start camera"); } // initialize camera
    dashboard.begin(); } else {
    DEBUG_MAIN_PRINTLN("[MODE] NORMAL MODE ACTIVATED.");
    sensors.begin(); // initialize sensors
    if(!camera.begin()){ DEBUG_MAIN_PRINTLN("[ERROR] Failed to start camera"); } // initialize camera
    dashboard.beginWiFi(); // wifi only
    DEBUG_MAIN_PRINT("[ACTION] Connecting to WiFi in setup");
    int connection_timeout = 30; // wait for ~15 seconds
    while (WiFi.status() != WL_CONNECTED && connection_timeout > 0) {
        delay(500); DEBUG_MAIN_PRINT(".");
        connection_timeout--;
    }

    // Initialize MQTT only if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_MAIN_PRINTLN("\n[INFO] WiFi connected successfully in setup."); 
        mqtt.onDataRequest(handleDataRequest); // register the data request callback
        mqtt.onConfig(handleConfig); // register the config callback
        mqtt.begin(); // initialize mqtt client
    } else {
        DEBUG_MAIN_PRINTLN("\n[WARN] WiFi connection failed in setup."); }
  }

  pinMode(SETUP_BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  if (isSetupMode) {
    // --- Setup Mode Loop ---
    dashboard.loop(); // continuously handle web server requests

    // check for a long button press to exit setup mode
    static unsigned long buttonPressStartTime = 0; bool buttonPressed = (digitalRead(SETUP_BUTTON_PIN) == LOW);
    if (buttonPressed) {
      DEBUG_MAIN_PRINTLN("[DEBUG] button press detected!"); if (buttonPressStartTime == 0) { buttonPressStartTime = millis(); }
      else if (millis() - buttonPressStartTime > 3000) {
        DEBUG_MAIN_PRINTLN("[ACTION] Exiting Setup mode via button press. Restarting..."); ESP.restart();
      }
    } else {
      buttonPressStartTime = 0; }

    if (millis() > SETUP_MODE_TIMEOUT) { // check for the 10-minute timeout
      DEBUG_MAIN_PRINTLN("[WARN] Setup mode timed out. Restarting..."); 
      preferences.putBool("setup_mode", false); // clear the flag for the next boot
      ESP.restart(); // reboot into normal mode
    }
  } else {
    // --- Normal Mode: Sense, Transmit, Sleep ---

    if (WiFi.status() == WL_CONNECTED) { // if wifi connected successfully
      DEBUG_MAIN_PRINTLN("[INFO] WiFi Connected."); // mqtt.loop() handles timed reconnection and returns status
      if (mqtt.loop()) {
          DEBUG_MAIN_PRINTLN("[INFO] MQTT broker connected."); 
          //sensors.readAllSensors(); // logic now handled by specific send functions
          //sendSensorData();
          
          unsigned long sense_interval = powerManager.getSenseInterval();
          unsigned long cam_interval = powerManager.getCamInterval();
          
          // Always send sensor data on wakeup in normal mode
          //sendSensorData();
          
          if (cam_interval > 0 && sense_interval > 0) { // prevent division by zero
            int sense_cycles_per_cam = cam_interval / sense_interval; 
            if (bootCount % sense_cycles_per_cam == 0) {
                //sendCameraData(); 
            }
          }
          delay(2000); // wait for data to be sent before sleeping
      } else {
          // MQTT connection failed after retries in MQTT.h
          DEBUG_MAIN_PRINTLN("\n[ERROR] Failed to connect to MQTT broker, entering setup mode on next boot.");
          preferences.putBool("setup_mode", true);
      }
      
    } else {
      DEBUG_MAIN_PRINTLN("\n[ERROR] Failed to connect to WiFi, entering setup mode on next boot.");
      preferences.putBool("setup_mode", true);
    }

    unsigned long sleep_duration_seconds = powerManager.getSenseInterval(); // get the sleep duration
    DEBUG_MAIN_PRINT("[ACTION] Entering deep sleep for ");
    DEBUG_MAIN_PRINT(sleep_duration_seconds); DEBUG_MAIN_PRINTLN(" seconds."); // configure wakeup sources
    esp_sleep_enable_timer_wakeup(sleep_duration_seconds * 1000000ULL); // set the wakeup timer
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // enable wakeup on button press

    esp_deep_sleep_start(); // enter deep sleep
  }
}