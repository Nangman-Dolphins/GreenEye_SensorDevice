#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> 

// --- Config: WiFi & MQTT ---
const char* WIFI_SSID = "*";
const char* WIFI_PASS = "*";
const char* MQTT_BROKER_IP = "*";

// Target topic for this device
const char* MQTT_TOPIC_SUB = "GreenEye/test/data";

// --- Globals ---
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// --- MQTT Message Callback ---
void messageReceivedCallback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("RX: Topic [%s]\n", topic);

    // prepare a JsonDocument.
    JsonDocument doc;

    // deserialize the payload into the document.
    DeserializationError error = deserializeJson(doc, payload, length);

    // check for parsing errors.
    if (error) {
        Serial.printf("FATAL: deserializeJson() failed: %s\n", error.c_str());
        return;
    }

    // extract data using keys.
    const char* configId = doc["config_id"];
    const char* resolution = doc["settings"]["resolution"];
    int jpegQuality = doc["settings"]["jpeg_quality"].as<int>();
    bool enableLed = doc["settings"]["enable_led"].as<bool>();
    const char* action = doc["action"];

    // use the data
    Serial.println("--- Parsed Config ---");
    Serial.printf("  Config ID: %s\n", configId);
    Serial.printf("  Resolution: %s\n", resolution);
    Serial.printf("  JPEG Quality: %d\n", jpegQuality);
    Serial.printf("  LED Enabled: %s\n", enableLed ? "true" : "false");
    Serial.printf("  Action: %s\n", action);
    Serial.println("---------------------\n");
}

// --- MQTT Reconnect Logic ---
void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("ge-sd-*")) { // Client ID
      Serial.println("OK.");
      // Subscribe to the target topic
      mqttClient.subscribe(MQTT_TOPIC_SUB);
    } else {
      Serial.printf("failed, rc=%d. Retrying in 5s...\n", mqttClient.state());
      delay(5000);
    }
  }
}

// --- Standard Setup & Loop ---
void setup() {
  Serial.begin(115200);

  // WiFi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi OK.");

  // MQTT Setup
  mqttClient.setServer(MQTT_BROKER_IP, 1883);
  mqttClient.setCallback(messageReceivedCallback); // Set the handler
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop(); // Handles MQTT background tasks
}