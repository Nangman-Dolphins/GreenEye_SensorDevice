#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_bt_main.h"

#include <WiFi.h>
#include <HTTPClient.h>

#define DATA_SAMPLE 10 // How many times will sense it
#define DATA_NO     20 // How many data to send
#define SENSER_TYPE "SoilEC_*"
#define SHEET_NAME  SENSER_TYPE

const float alpha = 0.05; // smoothing const (0.01 ~ 0.1)
float filteredValue = 0;

#define EC_PIN1     12
#define EC_PIN2     13
#define EC_READ_PIN 2

// --- WiFi Settings ---
const char* ssid = "*";
const char* password = "*";

// --- Google Script URL ---
// Replace this with the Web App URL you got from Google Apps Script deployment.
String scriptUrl = "*";
String sheetName = SHEET_NAME; 

float dataArr[DATA_NO] = {0,};

float readEC() {
  // EC_PIN1 -> EC_PIN2
  digitalWrite(EC_PIN1, HIGH);
  digitalWrite(EC_PIN2, LOW);
  delayMicroseconds(250);
  int val = analogRead(EC_READ_PIN);

  // EC_PIN1 <- EC_PIN2
  digitalWrite(EC_PIN1, LOW);
  digitalWrite(EC_PIN2, HIGH);
  delayMicroseconds(250); 
  val += analogRead(EC_READ_PIN);

  digitalWrite(EC_PIN1, LOW);
  digitalWrite(EC_PIN2, LOW);

  return (val / 2.0);
  
}

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
  pinMode(EC_PIN1, OUTPUT);
  pinMode(EC_PIN2, OUTPUT);


  Serial.print("Sensor Stabilizing ");

  for(int j = 0; j < 10; j++) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("Done!");

  Serial.println("Sensor start sensing ...");
  
  filteredValue = readEC();

  for(int i = 0; i < DATA_NO; i++){
    float rawValue = 0;

    for(int j = 0; j < DATA_SAMPLE; j++) {
      rawValue += readEC();
      delay(10);
    }
    filteredValue = alpha * (rawValue/DATA_SAMPLE) + (1.0 - alpha) * filteredValue;

    Serial.printf("  => %s[%d]: %f\n", SHEET_NAME, i, filteredValue);
    dataArr[i] = filteredValue;

    delay(500);
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