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

int cycle = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("=== Analog data archiver ===");

  Serial.println("Sensor Initializing ...");
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH);

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
  int analog_data = 0;

  for(int i = 0; i < DATA_SAMPLE; i++){
    analog_data += analogRead(SENS_PIN);
    delay(50);
  }
  analog_data = analog_data / DATA_SAMPLE;
  Serial.print("Done sensing : ");
  Serial.println(analog_data);

  HTTPClient http;
  // Construct the final URL with all parameters.
  String url = scriptUrl + "?sheet=" + sheetName + "&Sensor_Type="+ SENSER_TYPE + "&Value1=" + String(analog_data);
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

  cycle++;
  if(cycle > DATA_NO){
    Serial.println("Archive END.");
    while(true);
  }


  delay(1000); 
}