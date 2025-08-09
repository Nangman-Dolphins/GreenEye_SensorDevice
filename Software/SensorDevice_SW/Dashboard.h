#pragma once // Prevents multiple inclusion of the header file

#include <WiFi.h>      // for wifi functionality
#include <WebServer.h> // for the web server
#include <ESPmDNS.h>   // for .local domain names
#include <Preferences.h> // for non-volatile storage
#include <pgmspace.h>  // for storing data in flash memory
#include "Camera.h"

// --- Webpage HTML & CSS stored in PROGMEM ---
static const char DASHBOARD_MAIN_TEMPLATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="ko"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>ESP32 Dashboard</title>
<style>
body{font-family:Arial,sans-serif;display:flex;flex-direction:column;justify-content:center;align-items:center;height:100%;background-color:#f0f2f5;margin:0;padding-top:1rem;padding-bottom:1rem;width:100%;box-sizing:border-box;}
.container{margin:0.5rem;background-color:#fff;padding:2rem;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,.1);text-align:center;width:90%;max-width:420px;box-sizing:border-box;}
h1{color:#333;line-height:1.4;margin-top:0;}
.form-group{margin-bottom:1rem;text-align:left}
label{display:block;margin-bottom:.5rem;font-weight:700;color:#555}
input[type=text],input[type=password]{width:100%;padding:10px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box}
button{width:100%;padding:12px;color:#fff;border:none;border-radius:4px;font-size:1rem;cursor:pointer;background-color:#128037}
button:hover{background-color:#007025}
.status-box{text-align:left;padding:1.5rem;border:1px solid #e0e0e0;border-radius:8px;margin-bottom:1.5rem}
.status-item{display:flex;justify-content:space-between;align-items:center;margin-bottom:1rem}
.status-label{font-weight:700;color:#333}
.status-value{color:#555}
.status-connected{color:#28a745;font-weight:700}
.btn-danger{background-color:#dc3545}
.btn-danger:hover{background-color:#c82333}
a{margin:0.5rem;width:30%; text-decoration:none;}
.nav{display:flex;align-items:center;flex-direction:row;justify-content:center;}
.nav-button{background-color: #666666;}
.nav-button:hover{background-color: #333;}
</style>
</head><body><div class="container"><h1>__DASHBOARD_TITLE__</h1><br><div class="nav"><a href="/"><button class="nav-button">WiFi 설정</button></a><a href="/dashboard"><button class="nav-button">대시보드</button></a><a href="/camera"><button class="nav-button">카메라</button></a></div></div>
<div class="container">__PAGE_CONTENT__</div>
</body></html>
)rawliteral";

static const char STATUS_DASHBOARD_CONTENT[] PROGMEM = R"rawliteral(
<div class="status-box"><div class="status-item"><span class="status-label">연결 상태:</span><span class="status-value status-connected">연결됨</span></div><div class="status-item"><span class="status-label">WiFi 이름 (SSID):</span><span class="status-value">__CURRENT_SSID__</span></div></div>
<form action="/forget" method="post"><button type="submit" class="btn-danger">WiFi 정보 삭제</button></form>
)rawliteral";

static const char SETUP_FORM_CONTENT[] PROGMEM = R"rawliteral(
<form action="/save" method="post"><div class="form-group"><label for="ssid">WiFi 이름 (SSID)</label><input type="text" id="ssid" name="ssid" required></div><div class="form-group"><label for="password">비밀번호</label><input type="password" id="password" name="password"></div><button type="submit">저장 및 재부팅</button></form>
)rawliteral";

static const char DASHBOARD_CONTENT[] PROGMEM = R"rawliteral(
<div class="status-box">
    <div class="status-item"><span class="status-label">환경 온도:</span><span class="status-value">__TEMP_AMBIENT__ &deg;C</span></div>
    <div class="status-item"><span class="status-label">환경 습도:</span><span class="status-value">__HUMIDITY__ %</span></div>
    <div class="status-item"><span class="status-label">광도:</span><span class="status-value">__LIGHT__ lux</span></div>
    <hr style="border:none;border-top:1px solid #e0e0e0;margin:1rem 0;">
    <div class="status-item"><span class="status-label">토양 온도:</span><span class="status-value">__TEMP_SOIL__ &deg;C</span></div>
    <div class="status-item"><span class="status-label">토양 수분:</span><span class="status-value">__MOISTURE__ %</span></div>
    <div class="status-item"><span class="status-label">토양 전도도:</span><span class="status-value">__EC__ uS/cm</span></div>
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

static const char CAMERA_CONTENT[] PROGMEM = R"rawliteral(
<div>
    <img src="/stream" style="width:100%; max-width:400px; border-radius:8px;">
</div>
)rawliteral";


class Dashboard {
private:
    WebServer _server;          // web server object
    Preferences _preferences;   // non-volatile storage handler
    String _hostname;           // holds the device hostname, e.g. "ge-sd-XXXX"
    String _ap_password;        // holds the ap password
    bool _debug_enabled;        // flag for debug mode

    // pointers to external sensor data variables
    float* _p_temp_ambient;
    float* _p_humidity;
    float* _p_light;
    float* _p_temp_soil;
    float* _p_moisture;
    float* _p_ec;

    // for camera handling
    Camera* _p_camera;

public:
    Dashboard(
        float* p_temp_ambient, float* p_humidity, float* p_light,
        float* p_temp_soil, float* p_moisture, float* p_ec,
        Camera* p_camera,
        const char* apPassword = "defaultPW", 
        bool debug = false
    ) : _server(80), // initialize server on port 80
        _ap_password(apPassword),        // set ap password
        _debug_enabled(debug),           // set debug mode
        _p_temp_ambient(p_temp_ambient), // store pointer to ambient temp
        _p_humidity(p_humidity),         // store pointer to humidity
        _p_light(p_light),               // store pointer to light
        _p_temp_soil(p_temp_soil),       // store pointer to soil temp
        _p_moisture(p_moisture),         // store pointer to moisture
        _p_ec(p_ec),                     // store pointer to ec
        _p_camera(p_camera)
    {}

    void begin() {
        String mac_address = WiFi.macAddress(); // get mac address
        String mac_suffix = mac_address.substring(12, 14) + mac_address.substring(15, 17); // get last 4 hex digits
        mac_suffix.toUpperCase(); // convert to uppercase
        _hostname = "ge-sd-" + mac_suffix; // create unique hostname

        _preferences.begin("wifi-creds", false); // initialize preferences storage
        WiFi.mode(WIFI_AP_STA); // set wifi to both ap and station mode
        
        WiFi.softAP(_hostname.c_str(), _ap_password.c_str()); // start the access point
        
        IPAddress apIP = WiFi.softAPIP(); // get the ap's ip address
        if (_debug_enabled) { // if debug mode is on
            Serial.println("--- Access Point Started ---"); // print ap status
            Serial.print("AP SSID: "); Serial.println(_hostname); // print ssid
            Serial.print("AP IP Address: "); Serial.println(apIP); // print ip address
            Serial.println("--------------------------");
        }

        setupWebServer(); // configure web server routes
        delay(100);       // short delay for stability
        
        if (MDNS.begin(_hostname.c_str())) { // start mDNS service with the hostname
            MDNS.addService("http", "tcp", 80); // advertise web server on port 80
            if (_debug_enabled) { // if debug mode is on
                Serial.println("mDNS responder started."); // log mDNS status
                Serial.print("Access the dashboard at: http://");
                Serial.print(_hostname);
                Serial.println(".local");
            }
        } else { // if mDNS fails
            if (_debug_enabled) { Serial.println("Error setting up mDNS responder!"); } // log the error
        }

        String saved_ssid = _preferences.getString("ssid", ""); // read saved ssid from storage
        if (saved_ssid != "") { // if ssid exists
            connectToWiFi(); // try to connect to the saved network
        } else { // if no ssid is saved
            if (_debug_enabled) { Serial.println("No saved STA credentials. Ready to be configured."); }
        }
    }

    void loop() {
        _server.handleClient(); // handle incoming web requests
    }

    bool isConnected() {
        return WiFi.status() == WL_CONNECTED; // return true if connected to a wifi network
    }

private:
    void connectToWiFi() {
        if (_debug_enabled) { Serial.println("Function called: connectToWiFi()"); } // log function call
        String ssid = _preferences.getString("ssid", ""); // get ssid from storage
        String password = _preferences.getString("password", ""); // get password from storage
        if (_debug_enabled) { Serial.print("Attempting to connect to STA network: "); Serial.println(ssid); }
        WiFi.begin(ssid.c_str(), password.c_str()); // start connection attempt
    }

    void setupWebServer() {
        if (_debug_enabled) { Serial.println("Function called: setupWebServer()"); } // log function call
        _server.on("/", HTTP_GET, std::bind(&Dashboard::handleRoot, this)); // route for root page
        _server.on("/save", HTTP_POST, std::bind(&Dashboard::handleSave, this)); // route for saving credentials
        _server.on("/forget", HTTP_POST, std::bind(&Dashboard::handleForget, this)); // route for forgetting credentials
        _server.on("/dashboard", HTTP_GET, std::bind(&Dashboard::handleDashboard, this)); // route for sensor dashboard
        _server.on("/camera", HTTP_GET, std::bind(&Dashboard::handleCameraPage, this));
        _server.on("/stream", HTTP_GET, std::bind(&Dashboard::handleStream, this));
        _server.onNotFound([this]() { // handler for any other page
            if (_debug_enabled) { Serial.println("Function called: onNotFound"); } // log 404
            _server.send(404, "text/plain", "404: Not Found"); // send 404 error
        });
        _server.begin(); // start the web server
        if (_debug_enabled) { Serial.println("HTTP server started."); }
    }

    void handleRoot() {
        if (_debug_enabled) { Serial.println("Function called: handleRoot()"); } // log function call
        String page_template = FPSTR(DASHBOARD_MAIN_TEMPLATE); // load main template from flash
        String page_content; // prepare string for page content
        
        String title = "GreenEye 센서단말<br>[" + _hostname + "]"; // create dynamic title
        page_template.replace("__DASHBOARD_TITLE__", title); // replace title placeholder

        if (isConnected()) { // if connected to a router
            page_content = FPSTR(STATUS_DASHBOARD_CONTENT); // load status page content
            String current_ssid = _preferences.getString("ssid", "N/A"); // get current ssid
            page_content.replace("__CURRENT_SSID__", current_ssid); // replace ssid placeholder
        } else { // if not connected
            page_content = FPSTR(SETUP_FORM_CONTENT); // load setup form content
        }

        page_template.replace("__PAGE_CONTENT__", page_content); // insert content into template
        _server.send(200, "text/html", page_template); // send the final page
    }

    void handleDashboard() {
        if (_debug_enabled) { Serial.println("Function called: handleDashboard()"); } // log function call
        String page_template = FPSTR(DASHBOARD_MAIN_TEMPLATE); // load main template from flash
        String page_content = FPSTR(DASHBOARD_CONTENT); // load dashboard content from flash

        String title = "GreenEye 센서단말<br>[" + _hostname + "]"; // create dynamic title
        page_template.replace("__DASHBOARD_TITLE__", title); // replace title placeholder
        
        // read values from pointers, format them to 1 decimal place, and replace placeholders
        if (_p_temp_ambient) page_content.replace("__TEMP_AMBIENT__", String(*_p_temp_ambient, 1));
        if (_p_humidity)     page_content.replace("__HUMIDITY__",     String(*_p_humidity, 1));
        if (_p_light)        page_content.replace("__LIGHT__",        String(*_p_light, 1));
        if (_p_temp_soil)    page_content.replace("__TEMP_SOIL__",    String(*_p_temp_soil, 1));
        if (_p_moisture)     page_content.replace("__MOISTURE__",     String(*_p_moisture, 1));
        if (_p_ec)           page_content.replace("__EC__",           String(*_p_ec, 1));
        
        page_template.replace("__PAGE_CONTENT__", page_content); // insert content into template
        _server.send(200, "text/html", page_template); // send the final page
    }

    void handleSave() {
        if (_debug_enabled) { Serial.println("Function called: handleSave()"); } // log function call
        _preferences.putString("ssid", _server.arg("ssid")); // save ssid to storage
        _preferences.putString("password", _server.arg("password")); // save password to storage
        _server.send_P(200, "text/html", SUCCESS_PAGE_HTML); // send success page
        delay(2000); // wait for page to be sent
        ESP.restart(); // restart the device
    }

    void handleForget() {
        if (_debug_enabled) { Serial.println("Function called: handleForget()"); } // log function call
        _preferences.clear(); // clear all saved credentials
        MDNS.end(); // stop mDNS service
        _server.send_P(200, "text/html", FORGET_SUCCESS_PAGE_HTML); // send forgotten page
        delay(2000); // wait for page to be sent
        ESP.restart(); // restart the device
    }

    void handleCameraPage() {
        if (_debug_enabled) { Serial.println("Function called: handleCameraPage()"); }
        String page_template = FPSTR(DASHBOARD_MAIN_TEMPLATE);
        String page_content = FPSTR(CAMERA_CONTENT);
        String title = "GreenEye 센서단말<br>[" + _hostname + "]";
        page_template.replace("__DASHBOARD_TITLE__", title);
        page_template.replace("__PAGE_CONTENT__", page_content);
        _server.send(200, "text/html", page_template);
    }

    void handleStream() {
        if (!_p_camera) {
            _server.send(503, "text/plain", "Camera not initialized");
            return;
        }

        WiFiClient client = _server.client();
        String response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
        _server.sendContent(response);

        while (true) {
            camera_fb_t *fb = _p_camera->captureFrame();
            if (!fb) {
                Serial.println("Camera capture failed");
                break;
            }

            client.print("--frame\r\n");
            client.print("Content-Type: image/jpeg\r\n");
            client.print("Content-Length: ");
            client.println(fb->len);
            client.println();
            client.write(fb->buf, fb->len);
            client.print("\r\n");

            _p_camera->releaseFrame(fb);

            if (!client.connected()) {
                break;
            }
        }
    }
};