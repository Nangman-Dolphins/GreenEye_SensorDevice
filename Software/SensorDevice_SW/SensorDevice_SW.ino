#include "Camera.h"       // include the camera class
#include "Dashboard.h"    // include the dashboard class
#include "SensorsIO.h"    // include the sensor io class
#include "PowerManager.h" // include the power manager class
#include "MQTT.h"         // include the mqtt client class
#include <Preferences.h>  // include for using the setup mode flag

// pin for entering Setup Mode
#define SETUP_BUTTON_PIN 0 // IO0 pin is used to enter setup mode

// global variables & objects 
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
SensorsIO sensors(             // create the sensor io object
    &battery_level,
    &ambient_temp,
    &ambient_humidity,
    &light_intensity,
    &soil_temp,
    &soil_moisture,
    &soil_ec
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
    "defaultPW", // access Point Password
    true         // debug Mode (true = ON, false = OFF)
);
Preferences preferences;       // create the preferences object for the flag

// Global flag for current mode
bool isSetupMode = false; // flag to determine the current operating mode
const unsigned long SETUP_MODE_TIMEOUT = 10 * 60 * 1000; // 10 minutes timeout for setup mode

// MQTT Event Handler Functions

// this function sends all current sensor data via mqtt
void sendSensorData() {
    if (!dashboard.isConnected()) { // check if connected to wifi
        Serial.println("Not connected to WiFi, cannot send data.");
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

// this function sends a captured image via mqtt
void sendCameraData() {
    if (!dashboard.isConnected()) return; // exit if not connected
    Serial.println("Capturing high quality frame to send...");
    camera_fb_t* fb = camera.captureFrameForAnalyze(); // capture a high quality frame
    if (fb) { // if frame capture was successful
        JsonDocument dataDoc; // create json document
        // note: for actual use, the frame buffer (fb->buf) should be Base64 encoded here
        dataDoc["plant_img"] = "base64-encoded-image-placeholder"; 
        String output; // create string for json
        serializeJson(dataDoc, output); // convert json to string
        mqtt.publishData(output); // publish the image data
        camera.releaseFrame(fb); // release the frame buffer
    } else {
        Serial.println("Failed to capture high quality frame for sending.");
    }
}

// this function is called when the ccu requests data
void handleDataRequest() {
    Serial.println("Data request received from CCU.");
    sensors.readAllSensors(); // read all sensor values first
    sendSensorData();       // then send the new data
}

// this function is called when the ccu sends new configurations
void handleConfig(JsonDocument& doc, PowerManager& pm) {
    Serial.println("Config received from CCU.");
    if (doc.containsKey("pwr_mode")) { // check if power mode is being set
        char pwr_mode = doc["pwr_mode"].as<char>(); // get the character value
        pm.setMode(pwr_mode); // set the new power mode
    }
    if (doc.containsKey("nht_mode")) { // check if night mode is being set
        bool nht_mode = doc["nht_mode"].as<bool>(); // get the boolean value
        pm.setNightMode(nht_mode); // set the new night mode
    }
}

void setup() {
  Serial.begin(115200); // initialize serial communication
  delay(1000); // wait for serial monitor to open
  bootCount++; // increment the deep sleep wake-up counter
  Serial.printf("\n--- Boot count: %d ---\n", bootCount);

  pinMode(SETUP_BUTTON_PIN, INPUT); // set up the mode selection button pin
  preferences.begin("device-state", false); // initialize preferences for the mode flag
  bool enter_setup_flag = preferences.getBool("setup_mode", false); // check if the flag is set

  if (enter_setup_flag) { // if the flag was set before rebooting
    isSetupMode = true; // enter setup mode
    preferences.putBool("setup_mode", false); // clear the flag for the next boot
  } else { // if no flag, check the button press at boot time
    if (digitalRead(SETUP_BUTTON_PIN) == HIGH) { // check if button is held high
      isSetupMode = true; // enter setup mode
    }
  }

  // initialize subsystems based on the selected mode
  if (isSetupMode) {
    Serial.println("SETUP MODE ACTIVATED.");
    if(!camera.begin()){ Serial.println("Failed to start camera"); } // initialize camera
    dashboard.begin(); // initialize dashboard (web server)
  } else {
    Serial.println("NORMAL MODE ACTIVATED.");
    sensors.begin(); // initialize sensors
    if(!camera.begin()){ Serial.println("Failed to start camera"); } // initialize camera
    dashboard.begin(); // begin dashboard to load wifi/ccu settings and connect
    mqtt.onDataRequest(handleDataRequest); // register the data request callback
    mqtt.onConfig(handleConfig);           // register the config callback
    mqtt.begin(); // initialize mqtt client
  }
}

void loop() {
  if (isSetupMode) {
    // --- Setup Mode Loop ---
    dashboard.loop(); // continuously handle web server requests
    if (millis() > SETUP_MODE_TIMEOUT) { // check for the 10-minute timeout
      Serial.println("Setup mode timed out. Restarting...");
      ESP.restart(); // reboot into normal mode
    }
  } else {
    // --- Normal Mode: Connect, Sense, Transmit, Sleep ---
    Serial.println("Connecting to WiFi...");
    int connection_timeout = 20; // ~10 seconds
    while (WiFi.status() != WL_CONNECTED && connection_timeout > 0) { // wait for wifi connection
      delay(500);
      Serial.print(".");
      connection_timeout--;
    }

    if (WiFi.status() == WL_CONNECTED) { // if wifi connected successfully
      Serial.println("\nWiFi Connected.");
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
      delay(1000); // wait for data to be sent before sleeping
    } else {
      Serial.println("\nFailed to connect to WiFi.");
    }

    unsigned long sleep_duration_seconds = powerManager.getSenseInterval(); // get the sleep duration
    Serial.printf("Entering deep sleep for %lu seconds.\n", sleep_duration_seconds);
    esp_sleep_enable_timer_wakeup(sleep_duration_seconds * 1000000ULL); // set the wakeup timer
    esp_deep_sleep_start(); // enter deep sleep
  }
}