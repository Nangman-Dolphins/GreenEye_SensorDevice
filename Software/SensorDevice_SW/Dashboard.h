#pragma once // Prevents multiple inclusion of the header file

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <pgmspace.h>

// --- Webpage HTML & CSS stored in PROGMEM ---
static const char DASHBOARD_MAIN_TEMPLATE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="ko"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>ESP32 Dashboard</title>
<style>
body{font-family:Arial,sans-serif;display:flex;flex-direction:column;justify-content:center;align-items:center;min-height:100vh;background-color:#f0f2f5;margin:0;width:100%;}
.container{margin:0.5rem;background-color:#fff;padding:2rem;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,.1);text-align:center;width:75%;max-width:420px}
h1{color:#333;line-height:1.4;}
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
</style>
</head><body><div class="container"><h1>__DASHBOARD_TITLE__</h1></div><div class="container">__PAGE_CONTENT__</div></body></html>
)rawliteral";

static const char STATUS_DASHBOARD_CONTENT[] PROGMEM = R"rawliteral(
<div class="status-box"><div class="status-item"><span class="status-label">연결 상태:</span><span class="status-value status-connected">연결됨</span></div><div class="status-item"><span class="status-label">WiFi 이름 (SSID):</span><span class="status-value">__CURRENT_SSID__</span></div></div>
<form action="/forget" method="post"><button type="submit" class="btn-danger">WiFi 정보 삭제</button></form>
)rawliteral";

static const char SETUP_FORM_CONTENT[] PROGMEM = R"rawliteral(
<form action="/save" method="post"><div class="form-group"><label for="ssid">WiFi 이름 (SSID)</label><input type="text" id="ssid" name="ssid" required></div><div class="form-group"><label for="password">비밀번호</label><input type="password" id="password" name="password"></div><button type="submit">저장 및 재부팅</button></form>
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



class Dashboard {
private:
    WebServer _server;
    Preferences _preferences;
    String _hostname;
    String _ap_password;
    bool _debug_enabled; // variable to control debug output

public:
    Dashboard(const char* apPassword = "defaultPW", bool debug = false) 
        : _server(80), _ap_password(apPassword), _debug_enabled(debug) {}

    void begin() {
        Serial.begin(115200);
        if (_debug_enabled) {
            Serial.println("\nFunction called: begin()");
            Serial.println("Booting Dashboard (ge-sd-XXXX)...");
        }

        String mac_address = WiFi.macAddress();
        String mac_suffix = mac_address.substring(12, 14) + mac_address.substring(15, 17);
        mac_suffix.toUpperCase();
        _hostname = "ge-sd-" + mac_suffix;

        _preferences.begin("wifi-creds", false);
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
                Serial.println("mDNS responder started.");
                Serial.print("Access the dashboard at: http://");
                Serial.print(_hostname);
                Serial.println(".local");
            }
        } else {
            if (_debug_enabled) Serial.println("Error setting up mDNS responder!");
        }

        String saved_ssid = _preferences.getString("ssid", "");
        if (saved_ssid != "") {
            connectToWiFi();
        } else {
            if (_debug_enabled) Serial.println("No saved STA credentials. Ready to be configured.");
        }
    }

    void loop() {
        _server.handleClient();
    }

    bool isConnected() {
        return WiFi.status() == WL_CONNECTED;
    }

private:
    void connectToWiFi() {
        if (_debug_enabled) Serial.println("Function called: connectToWiFi()");
        String ssid = _preferences.getString("ssid", "");
        String password = _preferences.getString("password", "");
        if (_debug_enabled) { Serial.print("Attempting to connect to STA network: "); Serial.println(ssid); }
        WiFi.begin(ssid.c_str(), password.c_str());
    }

    void setupWebServer() {
        if (_debug_enabled) Serial.println("Function called: setupWebServer()");
        _server.on("/", HTTP_GET, std::bind(&Dashboard::handleRoot, this));
        _server.on("/save", HTTP_POST, std::bind(&Dashboard::handleSave, this));
        _server.on("/forget", HTTP_POST, std::bind(&Dashboard::handleForget, this));
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
            page_content = FPSTR(STATUS_DASHBOARD_CONTENT);
            String current_ssid = _preferences.getString("ssid", "N/A");
            page_content.replace("__CURRENT_SSID__", current_ssid);
        } else {
            page_content = FPSTR(SETUP_FORM_CONTENT);
        }

        page_template.replace("__PAGE_CONTENT__", page_content);
        _server.send(200, "text/html", page_template);
    }

    void handleSave() {
        if (_debug_enabled) Serial.println("Function called: handleSave()");
        _preferences.putString("ssid", _server.arg("ssid"));
        _preferences.putString("password", _server.arg("password"));
        _server.send_P(200, "text/html", SUCCESS_PAGE_HTML);
        delay(2000); 
        ESP.restart();
    }

    void handleForget() {
        if (_debug_enabled) Serial.println("Function called: handleForget()");
        _preferences.clear();
        MDNS.end();
        _server.send_P(200, "text/html", FORGET_SUCCESS_PAGE_HTML);
        delay(2000);
        ESP.restart();
    }
};