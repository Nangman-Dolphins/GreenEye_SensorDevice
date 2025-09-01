#pragma once // prevents multiple inclusion of the header file

#include <WiFi.h>      // for wifi functionality
#include <WebServer.h> // for the web server
#include <ESPmDNS.h>   // for .local domain names
#include <Preferences.h> // for non-volatile storage
#include <pgmspace.h>  // for storing data in flash memory
#include "Camera.h"    // include the custom camera class
#include "PowerManager.h" // include PowerManager to access its functions

// define function pointer types for actions triggered from the dashboard
using ActionCallback = void(*)();

// --- HTML Content Pages ---
static const char DASHBOARD_MAIN_TEMPLATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="ko"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>ESP32 Dashboard</title>
<style>
body{font-family:Arial,sans-serif;display:flex;flex-direction:column;justify-content:center;align-items:center;height:100%;background-color:#f0f2f5;margin:0;padding-top:1rem;padding-bottom:1rem;width:100%;box-sizing:border-box;}
.container{margin:0.5rem;background-color:#fff;padding:2rem;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,.1);text-align:center;width:90%;max-width:420px;box-sizing:border-box;}
h1{color:#333;line-height:1.2;margin-top:0;margin-bottom:0.5rem;} h3{margin-top:0;}
.form-group{margin-bottom:1.5rem;text-align:left}
label{display:block;margin-bottom:.5rem;font-weight:700;color:#555}
input[type=text],input[type=password],select{width:100%;padding:10px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box}
button{width:100%;padding:12px;color:#fff;border:none;border-radius:4px;font-size:1rem;cursor:pointer;background-color:#128037}
button:hover{background-color:#007025}
.status-box{text-align:left;padding:1.5rem;border:1px solid #e0e0e0;border-radius:8px;}
.status-item{display:flex;justify-content:space-between;align-items:center;margin-bottom:1rem}
.status-label{font-weight:700;color:#333}
.status-value{color:#555}
.status-connected{color:#28a745;font-weight:700}
.btn-danger{margin-top:1rem;background-color:#dc3545}
.btn-danger:hover{background-color:#c82333}
.btn-secondary{background-color:#6c757d} .btn-secondary:hover{background-color:#5a6268}
.btn-group form{margin-bottom:0.5rem;}
a{margin:0.2rem;width:24%; text-decoration:none;}
.nav{margin-top:-1rem;margin-bottom:-1rem;display:flex;align-items:center;flex-direction:row;justify-content:center;}
.nav-button{background-color:#666666;}
.nav-button:hover{background-color:#333;}
.battery-display{font-size:0.9rem;color:#333;margin-top:-0.5rem;margin-bottom:0.5rem;}
</style>
</head><body><div class="container"><div class="battery-display">배터리 잔량 __BATTERY_LEVEL__%</div><h1>__DASHBOARD_TITLE__</h1><br>
    <div class="nav"><a href="/"><button class="nav-button">연결</button></a><a href="/dashboard"><button class="nav-button">센서</button></a><a href="/camera"><button class="nav-button">카메라</button></a><a href="/debug"><button class="nav-button">디버그</button></a></div></div>
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
        <button type="submit">새 CCU 주소 저장</button>
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
    <h3>단말 정보</h3>
    <div class="status-item"><span class="status-label">전원 모드:</span><span class="status-value">__POWER_MODE__</span></div>
    <div class="status-item"><span class="status-label">야간 모드:</span><span class="status-value">__NIGHT_MODE__</span></div>
    <hr style="border:none;border-top:1px solid #e0e0e0;margin:1.5rem 0;">
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

static const char DEBUG_PAGE_CONTENT[] PROGMEM = R"rawliteral(
<div class="status-box">
    <h3>수동 제어</h3>
    <div class="btn-group">
        <form action="/send_sensor" method="post"><button type="submit">센싱 데이터 전송</button></form>
        <form action="/send_all" method="post"><button type="submit" class="btn-secondary">전체 데이터(센서+카메라) 전송</button></form>
    </div>
    <hr style="border:none;border-top:1px solid #e0e0e0;margin:1.5rem 0;">
    <h3>전원 모드 변경</h3>
    <form action="/set_power_mode" method="post">
        <div class="form-group">
            <select id="pwr_mode" name="pwr_mode">
                <option value="D">Debugging</option>
                <option value="U">Ultra High Freq</option>
                <option value="H">High Freq</option>
                <option value="M" selected>Normal</option>
                <option value="L">Low Power</option>
                <option value="Z">Ultra Low Power</option>
            </select>
        </div>
        <button type="submit">전원 모드 적용</button>
    </form>
    <hr style="border:none;border-top:1px solid #e0e0e0;margin:1.5rem 0;">
    <h3>야간 모드 변경</h3>
    <form action="/set_night_mode" method="post">
        <div class="form-group">
            <select id="nht_mode" name="nht_mode">
                <option value="1">활성화 (ON)</option>
                <option value="0">비활성화 (OFF)</option>
            </select>
        </div>
        <button type="submit">야간 모드 적용</button>
    </form>
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

static const char SAVE_SUCCESS_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="ko"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="refresh" content="3; url=/">
<title>설정 반영됨</title>
<style>
body{font-family:Arial,sans-serif;display:flex;justify-content:center;align-items:center;min-height:100vh;background-color:#f0f2f5;margin:0}
.container{background-color:#fff;padding:3rem;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,.1);text-align:center}
h1{color:#128037;margin-bottom:1rem;}
p{color:#555;font-size:1.1rem;}
</style></head>
<body><div class="container">
<h1>저장 완료!</h1><p>설정이 반영되었습니다.<br>잠시 후 메인 페이지로 돌아갑니다...</p>
</div></body></html>
)rawliteral";


class Dashboard {
private:
    WebServer _server;          // web server object
    Preferences _preferences;   // non-volatile storage handler
    String _hostname;           // holds the device hostname, e.g. "ge-sd-XXXX"
    String _ap_password;        // holds the ap password
    bool _debug_enabled;        // flag for debug mode

    int* _p_battery_level;    // pointer for battery level
    float* _p_temp_ambient;     // pointer for ambient temp
    float* _p_humidity;         // pointer for ambient humidity
    float* _p_light;            // pointer for light intensity
    float* _p_temp_soil;        // pointer for soil temperature
    float* _p_moisture;         // pointer for soil moisture
    float* _p_ec;               // pointer for soil conductivity
    Camera* _p_camera;          // pointer for camera object
    String* _p_ccu_address;     // pointer for ccu address string
    PowerManager* _p_power_manager;
    ActionCallback _sendSensorCallback;
    ActionCallback _sendAllCallback;


public:
    Dashboard(
        int* p_battery_level,
        float* p_temp_ambient, float* p_humidity, float* p_light,
        float* p_temp_soil, float* p_moisture, float* p_ec,
        Camera* p_camera,
        String* p_ccu_address,
        PowerManager* p_power_manager,
        ActionCallback sendSensorCallback,
        ActionCallback sendAllCallback,
        ActionCallback onDemandReadCallback,
        const char* apPassword = "defaultPW", 
        bool debug = false
    ) : _server(80),
        _ap_password(apPassword),
        _debug_enabled(debug),
        _p_battery_level(p_battery_level),
        _p_temp_ambient(p_temp_ambient),
        _p_humidity(p_humidity),
        _p_light(p_light),
        _p_temp_soil(p_temp_soil),
        _p_moisture(p_moisture),
        _p_ec(p_ec),
        _p_camera(p_camera),
        _p_ccu_address(p_ccu_address),
        _p_power_manager(p_power_manager), // Initialize power manager pointer
        _sendSensorCallback(sendSensorCallback), // Initialize sensor callback
        _sendAllCallback(sendAllCallback)      // Initialize all data callback
        _onDemandReadCallback(onDemandReadCallback)
    {}

    void begin() {
        Serial.begin(115200); // start serial communication
        if (_debug_enabled) { Serial.println("\n[INFO] Function called: begin()"); } // log function call

        String mac_address = WiFi.macAddress(); // get device mac address
        String mac_suffix = mac_address.substring(12, 14) + mac_address.substring(15, 17); // get last 4 hex digits
        mac_suffix.toUpperCase(); // convert to uppercase
        _hostname = "ge-sd-" + mac_suffix; // create unique hostname
        if (_debug_enabled) { Serial.print("[DEBUG] Hostname created: "); Serial.println(_hostname); }

        _preferences.begin("wifi-creds", false); // initialize preferences storage
        
        if (_p_ccu_address) { // if the ccu address pointer is valid
            *_p_ccu_address = _preferences.getString("ccu_address", ""); // load saved ccu address
        }

        WiFi.mode(WIFI_AP_STA); // set wifi to both ap and station mode
        WiFi.softAP(_hostname.c_str(), _ap_password.c_str()); // start the access point
        
        IPAddress apIP = WiFi.softAPIP(); // get the ap's ip address
        if (_debug_enabled) { // if debug mode is on
            Serial.println("--- Access Point Started ---"); // print ap status
            Serial.print("[INFO] AP SSID: "); Serial.println(_hostname); // print ssid
            Serial.print("[INFO] AP IP Address: "); Serial.println(apIP); // print ip address
            Serial.println("--------------------------");
        }

        setupWebServer(); // configure web server routes
        delay(100);       // short delay for stability
        
        if (MDNS.begin(_hostname.c_str())) { // start mDNS service with the hostname
            MDNS.addService("http", "tcp", 80); // advertise web server on port 80
            if (_debug_enabled) { // if debug mode is on
                Serial.println("[INFO] mDNS responder started."); // log mDNS status
                Serial.print("[INFO] Access dashboard at: http://");
                Serial.print(_hostname);
                Serial.println(".local");
            }
        } else { // if mDNS fails
             if (_debug_enabled) { Serial.println("[ERROR] Error setting up mDNS responder!"); } // log the error
        }
        
        String saved_ssid = _preferences.getString("ssid", ""); // read saved ssid from storage
        if (saved_ssid != "") { // if ssid exists
            connectToWiFi(); // try to connect to the saved network
        } else { // if no ssid is saved
            if (_debug_enabled) { Serial.println("[INFO] No saved STA credentials."); }
        }
    }

    void beginWiFi(bool connect_sta = true) {
        if (_debug_enabled) { Serial.println("[Dashboard] Initializing WiFi..."); }

        String mac_address = WiFi.macAddress();
        String mac_suffix = mac_address.substring(12, 14) + mac_address.substring(15, 17);
        mac_suffix.toUpperCase();
        _hostname = "ge-sd-" + mac_suffix;
        if (_debug_enabled) { // if debug mode is on
                Serial.print("[INFO] hostname is ");
                Serial.println(_hostname);
        }
        _preferences.begin("wifi-creds", false);
        if (_p_ccu_address) {
            *_p_ccu_address = _preferences.getString("ccu_address", "");
        }
        WiFi.mode(WIFI_STA);

        String saved_ssid = _preferences.getString("ssid", "");
        if (connect_sta && saved_ssid != "") {
            connectToWiFi();
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
        if (_debug_enabled) { Serial.println("[INFO] Function called: connectToWiFi()"); } // log function call
        String ssid = _preferences.getString("ssid", ""); // get ssid from storage
        String password = _preferences.getString("password", ""); // get password from storage
        if (_debug_enabled) { Serial.print("[DEBUG] Attempting to connect to STA network: "); Serial.println(ssid); }
        WiFi.begin(ssid.c_str(), password.c_str()); // start connection attempt
    }

    void setupWebServer() {
        if (_debug_enabled) { Serial.println("[INFO] Function called: setupWebServer()"); } // log function call
        _server.on("/", HTTP_GET, std::bind(&Dashboard::handleRoot, this)); // route for root page
        _server.on("/save", HTTP_POST, std::bind(&Dashboard::handleSave, this)); // route for saving wifi credentials
        _server.on("/forget", HTTP_POST, std::bind(&Dashboard::handleForget, this)); // route for forgetting wifi credentials
        _server.on("/dashboard", HTTP_GET, std::bind(&Dashboard::handleDashboard, this)); // route for sensor dashboard
        _server.on("/camera", HTTP_GET, std::bind(&Dashboard::handleCameraPage, this)); // route for camera page
        _server.on("/stream", HTTP_GET, std::bind(&Dashboard::handleStream, this)); // route for mjpeg stream
        _server.on("/save_ccu", HTTP_POST, std::bind(&Dashboard::handleSaveCcu, this)); // route for saving ccu address

        // ADDED: Routes for debug page actions
        _server.on("/debug", HTTP_GET, std::bind(&Dashboard::handleDebugPage, this));
        _server.on("/send_sensor", HTTP_POST, std::bind(&Dashboard::handleSendSensor, this));
        _server.on("/send_all", HTTP_POST, std::bind(&Dashboard::handleSendAll, this));
        _server.on("/set_power_mode", HTTP_POST, std::bind(&Dashboard::handleSetPowerMode, this));
        _server.on("/set_night_mode", HTTP_POST, std::bind(&Dashboard::handleSetNightMode, this));
        
        _server.onNotFound([this]() { // handler for any other page
            if (_debug_enabled) { Serial.println("[WARN] Function called: onNotFound - Page not found."); } // log 404
            _server.send(404, "text/plain", "404: Not Found"); // send 404 error
        });
        _server.begin(); // start the web server
        if (_debug_enabled) { Serial.println("[INFO] HTTP server started."); }
    }
    
    String buildPage(const char* content_progmem) {
        String page_template = FPSTR(DASHBOARD_MAIN_TEMPLATE); // load main template from flash
        String page_content = FPSTR(content_progmem); // load specific content from flash

        String title = "GreenEye 센서단말<br>[" + _hostname + "]"; // create dynamic title
        page_template.replace("__DASHBOARD_TITLE__", title); // replace title placeholder

        if (_p_battery_level) { // if battery pointer is valid
            page_template.replace("__BATTERY_LEVEL__", String(*_p_battery_level)); // replace battery placeholder
        } else { // if pointer is not valid
            page_template.replace("__BATTERY_LEVEL__", "N/A"); // show not available
        }

        if (content_progmem == DEVICE_STATUS_CONTENT) { // if it is the status page
            page_content.replace("__CURRENT_SSID__", _preferences.getString("ssid", "N/A")); // replace ssid
            String current_ccu = "Not Set"; // default text for ccu address
            if (_p_ccu_address && !(*_p_ccu_address).isEmpty()) { current_ccu = *_p_ccu_address; } // get value
            page_content.replace("__CURRENT_CCU_ADDRESS__", current_ccu); // replace ccu placeholder
        }
        else if (content_progmem == DASHBOARD_CONTENT) { // if it is the sensor dashboard page
            // ADDED: Replace device status placeholders
            if (_p_power_manager) {
                page_content.replace("__POWER_MODE__", _p_power_manager->getModeString());
                page_content.replace("__NIGHT_MODE__", _p_power_manager->isNightModeEnabled() ? "활성화" : "비활성화");
            }
            if (_p_temp_ambient) page_content.replace("__TEMP_AMBIENT__", String(*_p_temp_ambient, 1)); // update with sensor data
            if (_p_humidity)     page_content.replace("__HUMIDITY__",     String(*_p_humidity, 1));
            if (_p_light)        page_content.replace("__LIGHT__",        String(*_p_light, 1));
            if (_p_temp_soil)    page_content.replace("__TEMP_SOIL__",    String(*_p_temp_soil, 1));
            if (_p_moisture)     page_content.replace("__MOISTURE__",     String(*_p_moisture, 1));
            if (_p_ec)           page_content.replace("__EC__",           String(*_p_ec, 1));
        }
        
        page_template.replace("__PAGE_CONTENT__", page_content); // insert content into template
        return page_template; // return the complete html string
    }

    void handleRoot() {
        if (_debug_enabled) { Serial.println("[INFO] Function called: handleRoot()"); } // log function call
        const char* content = isConnected() ? DEVICE_STATUS_CONTENT : SETUP_FORM_CONTENT; // select content based on wifi status

        if (_onDemandReadCallback) {
            _onDemandReadCallback();
        }
        
        _server.send(200, "text/html", buildPage(content)); // build and send the page
    }

    void handleDashboard() {
        if (_debug_enabled) { Serial.println("[INFO] Function called: handleDashboard()"); } // log function call
        _server.send(200, "text/html", buildPage(DASHBOARD_CONTENT)); // build and send the page
    }

    void handleCameraPage() {
        if (_debug_enabled) { Serial.println("[INFO] Function called: handleCameraPage()"); } // log function call
        _server.send(200, "text/html", buildPage(CAMERA_CONTENT)); // build and send the page
    }

    void handleStream() {
        if (_debug_enabled) { Serial.println("[INFO] Function called: handleStream()"); } // log function call
        if (!_p_camera) { // if camera pointer is invalid
            if (_debug_enabled) { Serial.println("[ERROR] Stream failed: Camera not initialized."); }
            _server.send(503, "text/plain", "Camera not initialized"); // send error
            return;
        }

        WiFiClient client = _server.client(); // get the client
        String response = "HTTP/1.1 200 OK\r\n"; // prepare http response header
        response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n"; // specify mjpeg content type
        _server.sendContent(response); // send the header
        if (_debug_enabled) { Serial.println("[DEBUG] Stream started for client."); }

        while (client.connected()) { // loop to send frames
            camera_fb_t *fb = _p_camera->captureFrameForStream(); // capture a frame
            if (!fb) { // if capture fails
                if (_debug_enabled) { Serial.println("[ERROR] Camera capture failed during stream."); }
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
        }
        if (_debug_enabled) { Serial.println("[DEBUG] Stream ended for client."); }
    }

    void handleSaveCcu() {
        if (_debug_enabled) { Serial.println("[INFO] Function called: handleSaveCcu()"); }
        String new_ccu_address = _server.arg("ccu_address"); // get address from form
        if (_debug_enabled) { Serial.print("[DEBUG] Received new CCU address: "); Serial.println(new_ccu_address); }

        _preferences.putString("ccu_address", new_ccu_address); // save to non-volatile storage

        if (_p_ccu_address) { // if pointer is valid
            *_p_ccu_address = new_ccu_address; // update the variable in the main sketch
        }
        if (_debug_enabled) { Serial.println("[INFO] CCU address saved."); }
        _server.send_P(200, "text/html", SAVE_SUCCESS_PAGE_HTML); // send confirmation page
    }

    void handleSave() {
        if (_debug_enabled) { Serial.println("[INFO] Function called: handleSave()"); } // log function call
        
        String new_ssid = _server.arg("ssid"); // get ssid from form
        String new_password = _server.arg("password"); // get password from form
        String new_ccu_address = _server.arg("ccu_address"); // get ccu address from form

        _preferences.putString("ssid", new_ssid); // save ssid to storage
        _preferences.putString("password", new_password); // save password to storage
        _preferences.putString("ccu_address", new_ccu_address); // save ccu address to storage
        if (_p_ccu_address) { // if pointer is valid
            *_p_ccu_address = new_ccu_address; // update main sketch variable
        }
        
        if (_debug_enabled) {
            Serial.print("[DEBUG] Saved SSID: "); Serial.println(new_ssid);
            Serial.print("[DEBUG] Saved CCU Address: "); Serial.println(new_ccu_address);
            Serial.println("[INFO] WiFi and CCU info saved. Restarting...");
        }

        _server.send_P(200, "text/html", SUCCESS_PAGE_HTML); // send success page
        delay(2000); // wait for page to be sent
        ESP.restart(); // restart the device
    }

    void handleForget() {
        if (_debug_enabled) { Serial.println("[INFO] Function called: handleForget()"); } // log function call
        _preferences.clear(); // clear all saved credentials
        MDNS.end(); // stop mDNS service
        if (_debug_enabled) { Serial.println("[INFO] All preferences cleared. Restarting..."); }
        _server.send_P(200, "text/html", FORGET_SUCCESS_PAGE_HTML); // send forgotten page
        delay(2000); // wait for page to be sent
        ESP.restart(); // restart the device
    }
    
    // ADDED: Handlers for the new debug page
    void handleDebugPage() {
        if (_debug_enabled) { Serial.println("[INFO] Function called: handleDebugPage()"); }
        _server.send(200, "text/html", buildPage(DEBUG_PAGE_CONTENT));
    }

    void handleSendSensor() {
        if (_debug_enabled) { Serial.println("[ACTION] Triggered Send Sensor Data via dashboard."); }
        if (_sendSensorCallback) { _sendSensorCallback(); }
        _server.sendHeader("Location", "/debug");
        _server.send(302, "text/plain", "Sensor data sent. Redirecting...");
    }

    void handleSendAll() {
        if (_debug_enabled) { Serial.println("[ACTION] Triggered Send All Data via dashboard."); }
        if (_sendAllCallback) { _sendAllCallback(); }
        _server.sendHeader("Location", "/debug");
        _server.send(302, "text/plain", "All data sent. Redirecting...");
    }

    void handleSetPowerMode() {
        if (_p_power_manager && _server.hasArg("pwr_mode")) {
            String modeStr = _server.arg("pwr_mode");
            if (_debug_enabled) { Serial.print("[ACTION] Setting power mode to: "); Serial.println(modeStr[0]); }
            _p_power_manager->setMode(modeStr[0]);
        }
        _server.send_P(200, "text/html", SAVE_SUCCESS_PAGE_HTML); // send confirmation page
    }

    void handleSetNightMode() {
        if (_p_power_manager && _server.hasArg("nht_mode")) {
            bool nightMode = _server.arg("nht_mode").toInt() == 1;
            if (_debug_enabled) { Serial.print("[ACTION] Setting night mode to: "); Serial.println(nightMode ? "ON" : "OFF"); }
            _p_power_manager->setNightMode(nightMode);
        }
        _server.send_P(200, "text/html", SAVE_SUCCESS_PAGE_HTML); // send confirmation page
    }
};