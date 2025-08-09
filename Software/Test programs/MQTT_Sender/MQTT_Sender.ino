#include "WiFi.h"
#include "esp_camera.h"
#include "PubSubClient.h"

// --- CAMERA PIN DEFINITION (AI-THINKER MODEL) ---
// ** NOTE: You might need to change these pins depending on your specific ESP32-CAM model. **
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// --- WIFI and MQTT Configuration ---
const char* ssid = "*";         // <-- Enter your WiFi name here
const char* password = "*"; // <-- Enter your WiFi password here

// Enter the IP address
const char* mqtt_server = "*"; 
const int   mqtt_port = 1883;

// topic
const char* mqtt_topic = "esp32/cam/image";

// --- Global Variables ---
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
const int msg_interval_sec = 10; // Send an image every 10 seconds.

// --- Function to initialize the camera ---
bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Image quality settings.
  config.frame_size = FRAMESIZE_VGA; // 640x480
  config.jpeg_quality = 12; // 0-63, lower number means higher quality.
  config.fb_count = 1;

  // Initialize camera.
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return false;
  }
  return true;
}

// --- Function to reconnect to MQTT ---
void reconnect() {
  // Loop until reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID.
    String clientId = "ESP32-CAM-Client-";
    clientId += String(random(0xffff), HEX);
    
    // Attempt to connect.
    if (client.connect(clientId.c_str())) {
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying.
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // Initialize the camera.
  if (!initCamera()) {
    Serial.println("Failed to initialize camera! Restarting...");
    ESP.restart();
  }

  // Connect to WiFi.
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Configure MQTT server and port.
  client.setServer(mqtt_server, mqtt_port);
  // Increase buffer size to handle larger image payloads. Adjust if needed.
  client.setBufferSize(30 * 1024); // 30KB
}

void loop() {
  // Reconnect if the client is not connected.
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Required for the MQTT client to process messages.

  unsigned long now = millis();
  if (now - lastMsg > msg_interval_sec * 1000) {
    lastMsg = now;

    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();  // Take a picture.
    
    if (!fb) {
      Serial.println("Failed to get frame buffer.");
      return;
    }

    // Publish the captured image to the MQTT topic.
    bool published = client.publish(mqtt_topic, fb->buf, fb->len);
    
    if (published) {
      Serial.printf("Image sent successfully! (%d bytes)\n", fb->len);
    } else {
      Serial.println("Failed to send image.");
    }
    
    //return the frame buffer to memory to prevent leaks.
    esp_camera_fb_return(fb); 
  }
}