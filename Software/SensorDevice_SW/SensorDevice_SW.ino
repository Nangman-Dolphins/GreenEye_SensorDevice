#include "Camera.h"       // include the camera class
#include "Dashboard.h"    // include the dashboard class
#include "SensorsIO.h"    // include the sensor io class
#include "PowerManager.h" // include the power manager class
#include "MQTT.h"         // include the mqtt client class
#include <Preferences.h>  // include for using the setup mode flag

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

// --- Global Variables & Objects ---
RTC_DATA_ATTR int bootCount = 0; // a counter that survives deep sleep

int   battery_level    = 0;    // holds the battery level percentage
float ambient_temp     = 0.0;  // holds ambient temperature
float ambient_humidity = 0.0;  // holds ambient humidity
float light_intensity  = 0.0;  // holds light intensity
float soil_temp        = 0.0;  // holds soil temperature
float soil_moisture    = 0.0;  // holds soil moisture
float soil_ec          = 0.0;  // holds soil electrical conductivity
String ccu_address     = "";   // holds the ccu address

PowerManager powerManager;     // create the power manager object
Camera camera;                 // create the camera object
SensorIO sensors(             // create the sensor io object
    &battery_level,
    &ambient_temp,
    &ambient_humidity,
    &light_intensity,
    &soil_temp,
    &soil_moisture,
    &soil_ec,
    true
);
MQTTClient mqtt(&ccu_address, &powerManager); // create the mqtt client object
Dashboard dashboard(           // create the dashboard object
    &battery_level, 
    &ambient_temp, 
    &ambient_humidity, 
    &light_intensity, 
    &soil_temp, 
    &soil_moisture, 
    &soil_ec,
    &camera,
    &ccu_address,
    "defaultPW", // Access Point Password
    true         // set library debug mode to ON
);
Preferences preferences;       // create the preferences object for the flag

// --- Global flag for current mode ---
bool isSetupMode = false; // flag to determine the current operating mode
const unsigned long SETUP_MODE_TIMEOUT = 10 * 60 * 1000; // 10 minutes timeout for setup mode

// --- MQTT Event Handler Functions ---

void sendSensorData() {
    if (!dashboard.isConnected()) { // check if connected to wifi
        DEBUG_MAIN_PRINTLN("[WARN] Not connected to WiFi, cannot send data.");
        return; // exit if not connected
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
        JsonDocument dataDoc; // create json document
        // note: for actual use, the frame buffer (fb->buf) should be Base64 encoded here
        dataDoc["plant_img"] = "base64-encoded-image-placeholder"; 
        String output; // create string for json
        serializeJson(dataDoc, output); // convert json to string
        mqtt.publishData(output); // publish the image data
        camera.releaseFrameForAnalyze(fb); // release the frame buffer
    } else {
        DEBUG_MAIN_PRINTLN("[ERROR] Failed to capture high quality frame for sending.");
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

void setup() {
  Serial.begin(115200); // initialize serial communication
  delay(1000); // wait for serial monitor to open
  bootCount++; // increment the deep sleep wake-up counter
  DEBUG_MAIN_PRINT("[INFO] === Boot count: "); DEBUG_MAIN_PRINT(bootCount); DEBUG_MAIN_PRINTLN(" ===");

  pinMode(SETUP_BUTTON_PIN, INPUT); // set up the mode selection button pin
  preferences.begin("device-state", false); // initialize preferences for the mode flag
  bool enter_setup_flag = preferences.getBool("setup_mode", false); // check if the flag is set
  powerManager.begin();

  if (enter_setup_flag) { // if the flag was set before rebooting
    isSetupMode = true; // enter setup mode
    preferences.putBool("setup_mode", false); // clear the flag for the next boot
  } 

  // initialize subsystems based on the selected mode
  if (isSetupMode) {
    DEBUG_MAIN_PRINTLN("[MODE] SETUP MODE ACTIVATED.");
    if(!camera.begin()){ DEBUG_MAIN_PRINTLN("[ERROR] Failed to start camera"); } // initialize camera
    dashboard.beginWiFi();
    dashboard.beginWebServer(); // initialize dashboard (web server)
  } else {
    DEBUG_MAIN_PRINTLN("[MODE] NORMAL MODE ACTIVATED.");
    sensors.begin(); // initialize sensors
    if(!camera.begin()){ DEBUG_MAIN_PRINTLN("[ERROR] Failed to start camera"); } // initialize camera
    dashboard.beginWiFi();

    DEBUG_MAIN_PRINT("[ACTION] Connecting to WiFi in setup");
    int connection_timeout = 60; // wait for ~30 seconds
    while (WiFi.status() != WL_CONNECTED && connection_timeout > 0) {
        delay(500);
        DEBUG_MAIN_PRINT(".");
        connection_timeout--;
    }

    // Initialize MQTT only if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_MAIN_PRINTLN("\n[INFO] WiFi connected successfully in setup.");
        mqtt.onDataRequest(handleDataRequest); // register the data request callback
        mqtt.onConfig(handleConfig); // register the config callback
        mqtt.begin(); // initialize mqtt client
    } else {
        DEBUG_MAIN_PRINTLN("\n[WARN] WiFi connection failed in setup.");
    }
  }
}

void loop() {
  if (isSetupMode) {
    // --- Setup Mode Loop ---
    dashboard.loop(); // continuously handle web server requests

    // check for button press to exit setup mode
    static unsigned long buttonPressStartTime = 0;
    bool buttonPressed = (digitalRead(SETUP_BUTTON_PIN) == HIGH);
    if (buttonPressed) {
      DEBUG_MAIN_PRINTLN("[DEBUG] button press detected!");
      if (buttonPressStartTime == 0) { buttonPressStartTime = millis(); } 
      else if (millis() - buttonPressStartTime > 3000) {
        DEBUG_MAIN_PRINTLN("[ACTION] Exiting Setup mode via button press. Restarting...");
        ESP.restart();
      }
    } else {
      buttonPressStartTime = 0;
    }

    if (millis() > SETUP_MODE_TIMEOUT) { // check for the 10-minute timeout
      DEBUG_MAIN_PRINTLN("[WARN] Setup mode timed out. Restarting...");
      ESP.restart(); // reboot into normal mode
    }
  } else {
    // --- Normal Mode: Sense, Transmit, Sleep ---

    if (WiFi.status() == WL_CONNECTED) { // if wifi connected successfully
      DEBUG_MAIN_PRINTLN("[INFO] WiFi Connected.");
      mqtt.loop(); // connect to mqtt and process any initial messages
      
      sensors.readAllSensors(); // read all sensor values
      sendSensorData();       // send the sensor data via mqtt

      // check if it's time to capture and send a photo
      unsigned long sense_interval = powerManager.getSenseInterval();
      unsigned long cam_interval = powerManager.getCamInterval();
      if (cam_interval > 0 && sense_interval > 0) { // prevent division by zero
        int sense_cycles_per_cam = cam_interval / sense_interval; // calculate how many sensor cycles per camera cycle
        if (bootCount % sense_cycles_per_cam == 0) { // if it's time for a photo
            sendCameraData(); // capture and send the photo
        }
      }
      delay(2000); // wait for data to be sent before sleeping
    } else {
      DEBUG_MAIN_PRINTLN("\n[ERROR] Failed to connect to WiFi.");
      preferences.putBool("setup_mode", true);
    }

    unsigned long sleep_duration_seconds = powerManager.getSenseInterval(); // get the sleep duration
    DEBUG_MAIN_PRINT("[ACTION] Entering deep sleep for "); DEBUG_MAIN_PRINT(sleep_duration_seconds); DEBUG_MAIN_PRINTLN(" seconds.");
    esp_sleep_enable_timer_wakeup(sleep_duration_seconds * 1000000ULL); // set the wakeup timer
    esp_deep_sleep_start(); // enter deep sleep
  }
}