#pragma once // prevents multiple inclusion of the header file

#include "esp_camera.h" // esp32 camera library

// --- Camera Pin Configuration (for AI-Thinker ESP32-CAM) ---
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

#define DFT_QUALITY       15 // default quality for streaming
#define HIGH_QUALITY      5  // high quality for analysis


class Camera {
private:
    camera_config_t config; // holds the camera configuration
    sensor_t * s;

public:
    Camera() {
      // configure camera settings
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
      config.pixel_format = PIXFORMAT_JPEG; // use jpeg format for streaming
      config.fb_location = CAMERA_FB_IN_PSRAM;
      
      // set default resolution
      config.frame_size = FRAMESIZE_240X240; // 240x240 (default)
      config.jpeg_quality = DFT_QUALITY;  // 0-63, lower number means higher quality  (default)
      config.fb_count = 1;                // use 2 frame buffers for stability  (default)
    }

    bool begin() {
        // initialize the camera with the config
        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) { // if initialization fails
            Serial.printf("Camera init failed with error 0x%x", err);
            return false; // return false
        }

        // apply custom camera settings after successful init
        s = esp_camera_sensor_get();

        if (s) { // if sensor object is valid
            // --- Resolution & Quality ---
            s->set_framesize(s, FRAMESIZE_240X240);  // set frame size to default
            s->set_quality(s, DFT_QUALITY);       // set jpeg quality (default)

            // --- Lens Correction ---
            s->set_lenc(s, 1);                   // enable lens correction

            // --- Exposure & Brightness ---
            s->set_exposure_ctrl(s, 1);          // enable auto exposure control
            s->set_aec_value(s, 500);            // set exposure value
            s->set_gain_ctrl(s, 1);              // enable auto gain control
            s->set_agc_gain(s, 0);               // set gain level
            s->set_brightness(s, 1);             // set brightness

            // --- Color & White Balance ---
            s->set_whitebal(s, 1);               // enable auto white balance
            s->set_awb_gain(s, 1);               // enable auto white balance gain
            s->set_wb_mode(s, 0);                // set white balance mode to auto
            s->set_saturation(s, 1);             // set saturation
            s->set_contrast(s, 0);               // set contrast

            // --- Special Effects ---
            s->set_special_effect(s, 0);         // no special effects
            
            Serial.println("Custom camera settings applied.");
        } else {
            Serial.println("Failed to get camera sensor.");
        }
        esp_camera_fb_return(NULL);
        return true; // return true on success
    }

    camera_fb_t* captureFrameForStream() {
        // capture a single frame from the camera
        return esp_camera_fb_get();
    }

    void releaseFrameForStream(camera_fb_t* fb) {
        // return the frame buffer to be reused
        esp_camera_fb_return(fb);
    }

    camera_fb_t* captureFrameForAnalyze() {
        s->set_quality(s, HIGH_QUALITY);              // set jpeg quality (high)
        // capture a single frame from the camera
        return esp_camera_fb_get();
    }

    void releaseFrameForAnalyze(camera_fb_t* fb) {
        s->set_quality(s, DFT_QUALITY);               // reset to jpeg quality (default)

        // return the frame buffer to be reused
        esp_camera_fb_return(fb);
    }
};