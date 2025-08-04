#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_bt_main.h"

#include <WiFi.h>
#include <HTTPClient.h>

#define DATA_SAMPLE 10 // How many times will sense it
#define DATA_NO     10 // How many data to send
#define SHEET_NAME  "SoilMoist_*"
#define SENSER_TYPE "SoilMoist"
#define SENS_PIN    15 // IO15
#define EN_PIN      13 // IO13

// --- WiFi Settings ---
const char* ssid = "*";
const char* password = "*";

// --- Google Script URL ---
// Replace this with the Web App URL you got from Google Apps Script deployment.
String scriptUrl = "https://script.google.com/macros/s/AKfycbwUXMSMaR3Xw7uUeZySXcjaTjwx4_akqiASyt-9ovJFu7iFPSdEFohQVrkX64Xfw3Ko/exec";
String sheetName = SHEET_NAME; 

int dataArr[DATA_NO] = {0,};

void setup() {
  // Disable WiFI
  esp_wifi_stop();
  esp_wifi_deinit();

  // Disable BT
  esp_bt_controller_disable();
  esp_bt_controller_deinit();

  Serial.begin(115200);
  Serial.println("=== Analog data archiver ===");

  Serial.println("Sensor Initializing ...");
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH);

  for(int i = 0; i < DATA_NO; i++){
    int analog_data = 0;

    for(int j = 0; j < DATA_SAMPLE; j++){
      int temp = analogRead(SENS_PIN);
      Serial.printf("[%d] ", temp);
      analog_data += temp;
      delay(50);
    }
    analog_data = analog_data / DATA_SAMPLE;

    Serial.printf("  => %s[%d]: %d\n", SHEET_NAME, i, analog_data);
    dataArr[i] = analog_data;

    delay(200);
  }
  

  // --- Connect to WiFi ---
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void loop() {
  
  for(int k = 0; k < DATA_NO; k++){
    Serial.printf("[%d] Data > ", k);
    HTTPClient http;
    // Construct the final URL with all parameters.
    String url = scriptUrl + "?sheet=" + sheetName + "&Sensor_Type="+ SENSER_TYPE + "&Value1=" + String(dataArr[k]);
    Serial.print("Send request : ");
    Serial.println(url);

    http.begin(url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);


    int httpCode = http.GET();

    // Check the response.
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("HTTP Code: " + String(httpCode));
      Serial.println("Response: " + payload);
    } else {
      Serial.println("Error on HTTP request");
    }

    // Free up resources.
    http.end();
  }
  Serial.println("END");
  while(true);
}