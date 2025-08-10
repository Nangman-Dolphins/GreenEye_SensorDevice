#pragma once // prevents multiple inclusion of the header file

#include <WiFi.h>      // for wifi functionality
#include <WebServer.h> // for the web server
#include <ESPmDNS.h>   // for .local domain names
#include <Preferences.h> // for non-volatile storage
#include <pgmspace.h>  // for storing data in flash memory
#include "Camera.h"    // include the custom camera class

// --- HTML Content Pages ---
#pragma once // Prevents multiple inclusion of the header file

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <pgmspace.h>
#include "Camera.h"

// --- Webpage HTML & CSS stored in PROGMEM ---
static const char DASHBOARD_MAIN_TEMPLATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="ko"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>ESP32 Dashboard</title>
<style>
body{font-family:Arial,sans-serif;display:flex;flex-direction:column;justify-content:center;align-items:center;height:100%;background-color:#f0f2f5;margin:0;padding-top:1rem;padding-bottom:1rem;width:100%;box-sizing:border-box;}
.container{margin:0.5rem;background-color:#fff;padding:2rem;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,.1);text-align:center;width:90%;max-width:420px;box-sizing:border-box;}
h1{color:#333;line-height:1.2;margin-top:0;}
.form-group{margin-bottom:1.5rem;text-align:left}
label{display:block;margin-bottom:.5rem;font-weight:700;color:#555}
input[type=text],input[type=password]{width:100%;padding:10px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box}
button{width:100%;padding:12px;color:#fff;border:none;border-radius:4px;font-size:1rem;cursor:pointer;background-color:#128037}
button:hover{background-color:#007025}
.status-box{text-align:left;padding:1.5rem;border:1px solid #e0e0e0;border-radius:8px;}
.status-item{display:flex;justify-content:space-between;align-items:center;margin-bottom:1rem}
.status-label{font-weight:700;color:#333}
.status-value{color:#555}
.status-connected{color:#28a745;font-weight:700}
.btn-danger{margin-top:1rem;background-color:#dc3545}
.btn-danger:hover{background-color:#c82333}
a{margin:0.5rem;width:30%; text-decoration:none;}
.nav{margin-top:-1rem;margin-bottom:-1rem;display:flex;align-items:center;flex-direction:row;justify-content:center;}
.nav-button{background-color: #666666;}
.nav-button:hover{background-color: #333;}
</style>
</head><body><div class="container"><h1>__DASHBOARD_TITLE__</h1><br>
    <div class="nav"><a href="/"><button class="nav-button">연결 설정</button></a><a href="/dashboard"><button class="nav-button">센서 값</button></a><a href="/camera"><button class="nav-button">카메라</button></a></div></div>
<div class="container">__PAGE_CONTENT__</div></body></html>
)rawliteral";

static const char DEVICE_STATUS_CONTENT[] PROGMEM = R"rawliteral(
<div class="status-box">
    <div class="status-item"><span class="status-label">연결 상태:</span><span class="status-value status-connected">연결됨</span></div>
    <div class="status-item"><span class="status-label">WiFi 이름 (SSID):</span><span class="status-value">__CURRENT_SSID__</span></div>
    <form action="/forget" method="post"><button type="submit" class="btn-danger">WiFi 정보 삭제</button></form>
    <hr style="border:none;border-top:1px solid #e0e0e0;margin:2rem 0;">
    <div class="status-item"><span class="status-label">현재 CCU 주소:</span><span class="status-value">__CURRENT_CCU_ADDRESS__</span></div>
    <form action="/save_ccu" method="post">
        <div class="form-group"><hr style="border:none;border-top:0px solid #e0e0e0;margin:1rem 0;">
            <input type="text" id="ccu_address" name="ccu_address" placeholder="예: ge-sd-xxxx.local">
        </div>
        <button type="submit">새 CCU 주소 저장 및 연결</button>
    </form>
</div>
)rawliteral";

static const char SETUP_FORM_CONTENT[] PROGMEM = R"rawliteral(
<form action="/save" method="post">
    <div class="form-group"><label for="ssid">WiFi 이름 (SSID)</label><input type="text" id="ssid" name="ssid" required></div>
    <div class="form-group"><label for="password">비밀번호</label><input type="password" id="password" name="password"></div>
    <div class="form-group"><label for="ccu_address">CCU 주소 (선택 사항)</label><input type="text" id="ccu_address" name="ccu_address" placeholder="예: 192.168.0.100"></div>
    <button type="submit" style="margin-top:1rem;">저장 및 재부팅</button>
</form>
)rawliteral";

static const char DASHBOARD_CONTENT[] PROGMEM = R"rawliteral(
<div class="status-box">
    <h3>환경 정보</h3>
    <div class="status-item"><span class="status-label">온도:</span><span class="status-value">__TEMP_AMBIENT__ &deg;C</span></div>
    <div class="status-item"><span class="status-label">습도:</span><span class="status-value">__HUMIDITY__ %</span></div>
    <div class="status-item"><span class="status-label">광도:</span><span class="status-value">__LIGHT__ lux</span></div>
    <hr style="border:none;border-top:1px solid #e0e0e0;margin:1.5rem 0;">
    <h3>토양 정보</h3>
    <div class="status-item"><span class="status-label">온도:</span><span class="status-value">__TEMP_SOIL__ &deg;C</span></div>
    <div class="status-item"><span class="status-label">수분:</span><span class="status-value">__MOISTURE__ %</span></div>
    <div class="status-item"><span class="status-label">전도도:</span><span class="status-value">__EC__ uS/cm</span></div>
</div>
)rawliteral";

static const char CAMERA_CONTENT[] PROGMEM = R"rawliteral(
<div>
    <img src="/stream" style="width:100%; max-width:400px; border-radius:8px;">
</div>
)rawliteral";

static const char SUCCESS_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="ko"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="refresh" content="10; url=/">
<title>설정 저장됨</title>
<style>
body{font-family:Arial,sans-serif;display:flex;justify-content:center;align-items:center;min-height:100vh;background-color:#f0f2f5;margin:0}
.container{background-color:#fff;padding:3rem;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,.1);text-align:center}
.icon{width:50px;height:50px;margin-bottom:1rem;fill:#28a745}
h1{color:#28a745;margin-bottom:1rem;}
p{color:#555;font-size:1.1rem;}
</style></head>
<body><div class="container">
<svg class="icon" viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"></path></svg>
<h1>저장 완료!</h1><p>WiFi 정보가 저장되었습니다.<br>잠시 후(<span id="countdown">10</span>초) 메인 페이지로 이동합니다...</p>
</div>
<script>
  var countdownElement = document.getElementById('countdown');
  var seconds = 10;
  var interval = setInterval(function() {
    seconds--;
    countdownElement.textContent = seconds;
    if (seconds <= 0) {
      clearInterval(interval);
    }
  }, 1000);
</script>
</body></html>
)rawliteral";

static const char FORGET_SUCCESS_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="ko"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="refresh" content="10; url=/">
<title>정보 삭제됨</title>
<style>
body{font-family:Arial,sans-serif;display:flex;justify-content:center;align-items:center;min-height:100vh;background-color:#f0f2f5;margin:0}
.container{background-color:#fff;padding:3rem;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,.1);text-align:center}
.icon{width:50px;height:50px;margin-bottom:1rem;fill:#dc3545}
h1{color:#dc3545;margin-bottom:1rem;}
p{color:#555;font-size:1.1rem;}
</style></head>
<body><div class="container">
<svg class="icon" viewBox="0 0 24 24"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"></path></svg>
<h1>삭제 완료!</h1><p>저장된 WiFi 정보가 삭제되었습니다.<br>잠시 후(<span id="countdown">10</span>초) 메인 페이지로 이동합니다...</p>
</div>
<script>
  var countdownElement = document.getElementById('countdown');
  var seconds = 10;
  var interval = setInterval(function() {
    seconds--;
    countdownElement.textContent = seconds;
    if (seconds <= 0) {
      clearInterval(interval);
    }
  }, 1000);
</script>
</body></html>
)rawliteral";

static const char CCU_SAVE_SUCCESS_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="ko"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="refresh" content="10; url=/">
<title>CCU 주소 저장됨</title>
<style>
body{font-family:Arial,sans-serif;display:flex;justify-content:center;align-items:center;min-height:100vh;background-color:#f0f2f5;margin:0}
.container{background-color:#fff;padding:3rem;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,.1);text-align:center}
.icon{width:50px;height:50px;margin-bottom:1rem;fill:#28a745}
h1{color:#28a745;margin-bottom:1rem;}
p{color:#555;font-size:1.1rem;}
</style></head>
<body><div class="container">
<svg class="icon" viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"></path></svg>
<h1>저장 완료!</h1><p>CCU 주소가 저장되었습니다.<br>잠시 후(<span id="countdown">10</span>초) 메인 페이지로 이동합니다...</p>
</div>
<script>
  var countdownElement = document.getElementById('countdown');
  var seconds = 10;
  var interval = setInterval(function() {
    seconds--;
    countdownElement.textContent = seconds;
    if (seconds <= 0) {
      clearInterval(interval);
    }
  }, 1000);
</script>
</body></html>
)rawliteral";


class Dashboard {
private:
    WebServer _server;
    Preferences _preferences;
    String _hostname;
    String _ap_password;
    bool _debug_enabled;

    float* _p_temp_ambient;
    float* _p_humidity;
    float* _p_light;
    float* _p_temp_soil;
    float* _p_moisture;
    float* _p_ec;
    Camera* _p_camera;
    String* _p_ccu_address;

public:
    Dashboard(
        float* p_temp_ambient, float* p_humidity, float* p_light,
        float* p_temp_soil, float* p_moisture, float* p_ec,
        Camera* p_camera,
        String* p_ccu_address,
        const char* apPassword = "defaultPW", 
        bool debug = false
    ) : _server(80),
        _ap_password(apPassword),
        _debug_enabled(debug),
        _p_temp_ambient(p_temp_ambient),
        _p_humidity(p_humidity),
        _p_light(p_light),
        _p_temp_soil(p_temp_soil),
        _p_moisture(p_moisture),
        _p_ec(p_ec),
        _p_camera(p_camera),
        _p_ccu_address(p_ccu_address)
    {}

    void begin() {
        Serial.begin(115200);
        if (_debug_enabled) Serial.println("\nFunction called: begin()");

        String mac_address = WiFi.macAddress();
        String mac_suffix = mac_address.substring(12, 14) + mac_address.substring(15, 17);
        mac_suffix.toUpperCase();
        _hostname = "ge-sd-" + mac_suffix;

        _preferences.begin("wifi-creds", false);
        
        if (_p_ccu_address) {
            *_p_ccu_address = _preferences.getString("ccu_address", "");
        }

        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(_hostname.c_str(), _ap_password.c_str()); 
        
        IPAddress apIP = WiFi.softAPIP();
        if (_debug_enabled) {
            Serial.println("--- Access Point Started ---");
            Serial.print("AP SSID: "); Serial.println(_hostname);
            Serial.print("AP IP Address: "); Serial.println(apIP);
            Serial.println("--------------------------");
        }

        setupWebServer();
        delay(100);
        
        if (MDNS.begin(_hostname.c_str())) {
            MDNS.addService("http", "tcp", 80);
            if (_debug_enabled) {
                Serial.print("Access the dashboard at: http://");
                Serial.print(_hostname); Serial.println(".local");
            }
        }
        
        String saved_ssid = _preferences.getString("ssid", "");
        if (saved_ssid != "") {
            connectToWiFi();
        } else {
            if (_debug_enabled) Serial.println("No saved STA credentials.");
        }
    }

    void loop() { _server.handleClient(); }
    bool isConnected() { return WiFi.status() == WL_CONNECTED; }

private:
    void connectToWiFi() {
        if (_debug_enabled) { Serial.println("Function called: connectToWiFi()"); } // log function call
        String ssid = _preferences.getString("ssid", ""); // get ssid from storage
        String password = _preferences.getString("password", ""); // get password from storage
        if (_debug_enabled) { Serial.print("Attempting to connect to STA network: "); Serial.println(ssid); }
        WiFi.begin(ssid.c_str(), password.c_str()); // start connection attempt
    }

    void setupWebServer() {
        if (_debug_enabled) Serial.println("Function called: setupWebServer()");
        _server.on("/", HTTP_GET, std::bind(&Dashboard::handleRoot, this));
        _server.on("/save", HTTP_POST, std::bind(&Dashboard::handleSave, this));
        _server.on("/forget", HTTP_POST, std::bind(&Dashboard::handleForget, this));
        _server.on("/dashboard", HTTP_GET, std::bind(&Dashboard::handleDashboard, this));
        _server.on("/camera", HTTP_GET, std::bind(&Dashboard::handleCameraPage, this));
        _server.on("/stream", HTTP_GET, std::bind(&Dashboard::handleStream, this));
        _server.on("/save_ccu", HTTP_POST, std::bind(&Dashboard::handleSaveCcu, this));
        _server.onNotFound([this]() { 
            if (_debug_enabled) Serial.println("Function called: onNotFound");
            _server.send(404, "text/plain", "404: Not Found"); 
        });
        _server.begin();
        if (_debug_enabled) Serial.println("HTTP server started.");
    }

    void handleRoot() {
        if (_debug_enabled) Serial.println("Function called: handleRoot()");
        String page_template = FPSTR(DASHBOARD_MAIN_TEMPLATE);
        String page_content;
        String title = "GreenEye 센서단말<br>[" + _hostname + "]";
        page_template.replace("__DASHBOARD_TITLE__", title);

        if (isConnected()) {
            page_content = FPSTR(DEVICE_STATUS_CONTENT);
            String current_ssid = _preferences.getString("ssid", "N/A");
            page_content.replace("__CURRENT_SSID__", current_ssid);
            
            String current_ccu = "Not Set";
            if (_p_ccu_address && !(*_p_ccu_address).isEmpty()) {
                current_ccu = *_p_ccu_address;
            }
            page_content.replace("__CURRENT_CCU_ADDRESS__", current_ccu);

        } else {
            page_content = FPSTR(SETUP_FORM_CONTENT);
        }

        page_template.replace("__PAGE_CONTENT__", page_content);
        _server.send(200, "text/html", page_template);
    }

    void handleDashboard() {
        if (_debug_enabled) { Serial.println("Function called: handleDashboard()"); } // log function call
        String page_template = FPSTR(DASHBOARD_MAIN_TEMPLATE); // load main template from flash
        String page_content = FPSTR(DASHBOARD_CONTENT); // load dashboard content from flash

        String title = "GreenEye 센서단말<br>[" + _hostname + "]"; // create dynamic title
        page_template.replace("__DASHBOARD_TITLE__", title); // replace title placeholder
        
        if (_p_temp_ambient) page_content.replace("__TEMP_AMBIENT__", String(*_p_temp_ambient, 1)); // update with sensor data
        if (_p_humidity)     page_content.replace("__HUMIDITY__",     String(*_p_humidity, 1));
        if (_p_light)        page_content.replace("__LIGHT__",        String(*_p_light, 1));
        if (_p_temp_soil)    page_content.replace("__TEMP_SOIL__",    String(*_p_temp_soil, 1));
        if (_p_moisture)     page_content.replace("__MOISTURE__",     String(*_p_moisture, 1));
        if (_p_ec)           page_content.replace("__EC__",           String(*_p_ec, 1));
        
        page_template.replace("__PAGE_CONTENT__", page_content); // insert content into template
        _server.send(200, "text/html", page_template); // send the final page
    }

    void handleCameraPage() {
        if (_debug_enabled) { Serial.println("Function called: handleCameraPage()"); } // log function call
        String page_template = FPSTR(DASHBOARD_MAIN_TEMPLATE); // load main template
        String page_content = FPSTR(CAMERA_CONTENT); // load camera page content
        String title = "GreenEye 센서단말<br>[" + _hostname + "]"; // create dynamic title
        page_template.replace("__DASHBOARD_TITLE__", title); // replace placeholder
        page_template.replace("__PAGE_CONTENT__", page_content); // replace placeholder
        _server.send(200, "text/html", page_template); // send final page
    }

    void handleStream() {
        if (_debug_enabled) { Serial.println("Function called: handleStream()"); } // log function call
        if (!_p_camera) { // if camera pointer is invalid
            _server.send(503, "text/plain", "Camera not initialized"); // send error
            return;
        }

        WiFiClient client = _server.client(); // get the client
        String response = "HTTP/1.1 200 OK\r\n"; // prepare http response header
        response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n"; // specify mjpeg content type
        _server.sendContent(response); // send the header

        while (true) { // loop to send frames
            camera_fb_t *fb = _p_camera->captureFrameForStream(); // capture a frame
            if (!fb) { // if capture fails
                Serial.println("Camera capture failed");
                break; // exit the loop
            }

            client.print("--frame\r\n"); // print mjpeg boundary
            client.print("Content-Type: image/jpeg\r\n"); // specify content type
            client.print("Content-Length: "); // specify content length
            client.println(fb->len); // the length of the frame buffer
            client.println(); // empty line before content
            client.write(fb->buf, fb->len); // write the image data
            client.print("\r\n"); // empty line after content

            _p_camera->releaseFrameForStream(fb); // release the frame buffer

            if (!client.connected()) { // if client disconnects
                break; // exit the loop
            }
        }
    }

    void handleSaveCcu() {
        if (_debug_enabled) Serial.println("Function called: handleSaveCcu()");
        
        String new_ccu_address = _server.arg("ccu_address");
        if (_debug_enabled) { Serial.print("Saving new CCU address: "); Serial.println(new_ccu_address); }

        _preferences.putString("ccu_address", new_ccu_address);

        if (_p_ccu_address) {
            *_p_ccu_address = new_ccu_address;
        }

        _server.send_P(200, "text/html", CCU_SAVE_SUCCESS_PAGE_HTML);
    }

    void handleSave() {
        if (_debug_enabled) Serial.println("Function called: handleSave()");
        
        // Save WiFi credentials
        _preferences.putString("ssid", _server.arg("ssid"));
        _preferences.putString("password", _server.arg("password"));
        
        // Also save the CCU address from the setup form
        String new_ccu_address = _server.arg("ccu_address");
        _preferences.putString("ccu_address", new_ccu_address);
        if (_p_ccu_address) {
            *_p_ccu_address = new_ccu_address;
        }
        
        if (_debug_enabled) {
            Serial.print("Saved SSID: "); Serial.println(_server.arg("ssid"));
            Serial.print("Saved CCU Address: "); Serial.println(new_ccu_address);
        }

        _server.send_P(200, "text/html", SUCCESS_PAGE_HTML);
        delay(2000); 
        ESP.restart();
    }

    void handleForget() {
        if (_debug_enabled) { Serial.println("Function called: handleForget()"); } // log function call
        _preferences.clear(); // clear all saved credentials
        MDNS.end(); // stop mDNS service
        _server.send_P(200, "text/html", FORGET_SUCCESS_PAGE_HTML); // send forgotten page
        delay(2000); // wait for page to be sent
        ESP.restart(); // restart the device
    }
};