#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

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

// WiFi
const char* ssid = "*";
const char* password = "*";

WebServer server(80);

// shooting camera
void handle_jpg() {
  camera_fb_t * fb = NULL;
  
  // shooting!
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    server.send(500, "text/plain", "Failed to capture image");
    return;
  }

  server.sendHeader("Content-Type", "image/jpeg");
  server.sendHeader("Content-Disposition", "inline; filename=capture.jpg");

  // send image
  server.setContentLength(fb->len);
  server.send(200, "image/jpeg", ""); // 헤더 먼저 전송
  server.client().write(fb->buf, fb->len); // 이미지 버퍼 데이터 직접 전송
  
  // return camera frame buffer
  esp_camera_fb_return(fb);
}

// root page handler
void handle_root() {
  sensor_t * s = esp_camera_sensor_get();
  if (!s) {
    server.send(500, "text/plain", "Failed to get sensor.");
    return;
  }
  
  // read camera setting
  const char* resolution;
  switch (s->status.framesize) {
    case FRAMESIZE_UXGA:   resolution = "UXGA (1600x1200)"; break;
    case FRAMESIZE_XGA:    resolution = "XGA (1024x768)"; break;
    case FRAMESIZE_SVGA:   resolution = "SVGA (800x600)"; break;
    case FRAMESIZE_VGA:    resolution = "VGA (640x480)"; break;
    default:               resolution = "Unknown";
  }
  String html = R"rawliteral(
<!DOCTYPE html><html><head><title>ESP32-CAM TEST</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body { font-family: sans-serif; text-align: center; margin: 0; padding: 20px; background-color: #f4f4f4; }
#photo-container { max-width: 800px; margin: 0 auto; border: 1px solid #ddd; border-radius: 8px; overflow: hidden; }
img { display: block; width: 100%; height: auto; } .info { padding: 15px; background-color: #fff; }
button { margin-top: 20px; padding: 12px 25px; font-size: 16px; cursor: pointer; border: none; border-radius: 5px; background-color: #007bff; color: white; }
</style></head><body><h1>ESP32-CAM TEST</h1>
<div id="photo-container">
<img id="photo" src="/capture.jpg" alt="ESP32-CAM Capture">
<div class="info">)rawliteral";

  html += "<p><b>Resolution:</b> " + String(resolution) + "</p>";
  html += "<p><b>XCLK Frequency:</b> " + String((s->xclk_freq_hz)/1000000) + " MHz</p>";
  html += "<p><b>JPEG Quality:</b> " + String(s->status.quality) + "</p><br>";
  html += "<p><b>CPU Frequency:</b> " + String(getCpuFrequencyMhz()) + " MHz</p>";

  html += R"rawliteral(</div></div>
<a id="download-link" href="/capture.jpg" download="esp32-capture.jpg"><button>Store JPEG</button></a>
<script>
document.getElementById('photo').src = '/capture.jpg?' + new Date().getTime();
document.getElementById('download-link').href = document.getElementById('photo').src;
</script></body></html>)rawliteral";
  
  server.send(200, "text/html", html);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Brownout detector disable

  Serial.begin(115200);
  Serial.println("Booting...");

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
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA; // VGA(640x480), SVGA(800x600), XGA(1024x768), UXGA(1600x1200)
  config.jpeg_quality = 18; // (0-63, 0 is best quality)
  config.fb_count = 1;

  if (psramFound()) {
    Serial.println("PSRAM found. Overriding with high quality settings.");
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 13;
    config.fb_count = 1;
  }

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // tuning camera
  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 0);       // (1: on)
  s->set_hmirror(s, 0);     // (1: on)
  s->set_brightness(s, 0);  // (-2 ~ 2)
  s->set_contrast(s, 0);    // (-2 ~ 2)
  s->set_saturation(s, 0);  // (-2 ~ 2)
  s->set_awb_gain(s, 1);    // (1: on)

  // WiFi connect
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Register web server's handler
  server.on("/", HTTP_GET, handle_root);
  server.on("/capture.jpg", HTTP_GET, handle_jpg);
 
  // start server
  server.begin();

  Serial.print("> http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
}