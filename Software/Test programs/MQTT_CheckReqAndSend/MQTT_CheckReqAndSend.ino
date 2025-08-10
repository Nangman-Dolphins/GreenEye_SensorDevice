#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// config: WiFi & MQTT
const char* WIFI_SSID = "*";
const char* WIFI_PASS = "*";
const char* MQTT_BROKER_IP = "*";

// temporary, hardcoded topics and ID
const char* deviceId = "test";
const char* topicReq = "test/req";
const char* topicData = "test/data";

// globals
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// forward declaration
void publishSensorData();

// mqtt message callback
void messageReceivedCallback(char* topic, byte* payload, unsigned int length) {
    Serial.println("\n<-- Callback Triggered -->");
    Serial.printf("[callback] message arrived on topic: %s\n", topic);

    // check if the message is on our request topic
    if (strcmp(topic, topicReq) == 0) {
        Serial.println("[callback] topic matched. processing request...");
        
        // call the function to publish sensor data
        publishSensorData();
    } else {
        Serial.println("[callback] topic did not match. ignoring.");
    }
    Serial.println("<-- Callback Finished -->");
}

// sensor data publisher
void publishSensorData() {
    Serial.println("  [action] entering publishSensorData function...");
    
    // prepare a JSON document
    JsonDocument doc;
    Serial.println("  [action] json document created.");

    // simulate sensor readings
    doc["deviceId"] = deviceId;
    doc["value"] = random(100, 200);
    doc["timestamp"] = millis();
    Serial.println("  [action] simulated sensor data populated.");

    // serialize the JSON document to a string
    char jsonBuffer[128];
    serializeJson(doc, jsonBuffer);
    Serial.printf("  [action] json serialized: %s\n", jsonBuffer);

    // publish the JSON payload to the data topic
    Serial.printf("  [action] attempting to publish to topic: %s\n", topicData);
    if (mqttClient.publish(topicData, jsonBuffer)) {
        Serial.println("  [action] publish command sent successfully.");
    } else {
        Serial.println("  [action] ERROR: publish command failed.");
    }
}


// mqtt reconnect logic
void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("[mqtt] attempting connection to broker...");
    if (mqttClient.connect(deviceId)) {
      Serial.println(" OK.");
      
      // subscribe to the request topic
      mqttClient.subscribe(topicReq);
      Serial.printf("[mqtt] subscribed to topic: %s\n", topicReq);
    } else {
      Serial.printf(" failed, rc=%d. retrying in 5s...\n", mqttClient.state());
      delay(5000);
    }
  }
}

// standard setup & loop
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n[setup] device booting...");
    randomSeed(analogRead(0));

    // wiFi connection
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("[setup] connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println(" OK.");

    // mqtt setup
    mqttClient.setServer(MQTT_BROKER_IP, 1883);
    mqttClient.setCallback(messageReceivedCallback);
    Serial.println("[setup] mqtt client configured.");
    Serial.println("[setup] setup complete. entering main loop...");
}

void loop() {
    if (!mqttClient.connected()) {
        reconnect();
    }
    mqttClient.loop();
}