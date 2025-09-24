#pragma once // prevents multiple inclusion of the header file

#include <WiFi.h>        // for wifi functionality
#include <Preferences.h> // for non-volatile storage
#include <ESPmDNS.h>     // for .local domain names

// wifi and network management class
class NetworkManager {
private:
    Preferences *_p_preferences; // non-volatile storage for wifi credentials
    String _hostname;         // holds the device hostname

public:
    NetworkManager(Preferences* p_prefs) : _p_preferences(p_prefs) {}

    // initializes wifi in ap and sta mode
    void begin() {
      // generate unique hostname from mac address
      String mac_address = WiFi.macAddress();
      String mac_suffix = mac_address.substring(12, 14) + mac_address.substring(15, 17);
      mac_suffix.toUpperCase();
      _hostname = "ge-sd-" + mac_suffix;
      Serial.print("[INFO] Hostname created: ");
      Serial.println(_hostname);

      // start _p_preferences with "wifi-creds" namespace
      _p_preferences->begin("wifi-creds", true);
      String saved_ssid = _p_preferences->getString("ssid", "");
      String saved_password = _p_preferences->getString("password", "");
      _p_preferences->end();

      // start softap and station mode together
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP(_hostname.c_str(), "defaultPW");
      Serial.println("[INFO] SoftAP started.");
      Serial.print("[INFO] AP IP Address: ");
      Serial.println(WiFi.softAPIP());

      // if saved ssid exists, try to connect
      if (saved_ssid != "") {
        Serial.print("[INFO] Connecting to saved WiFi: ");
        Serial.println(saved_ssid);
        WiFi.begin(saved_ssid.c_str(), saved_password.c_str());

        int retries = 30; // try to connect for about 15 seconds
        while (WiFi.status() != WL_CONNECTED && retries > 0) {
          Serial.print(".");
          delay(500);
          retries--;
        }

        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("\n[INFO] WiFi connected!");
          Serial.print("[INFO] IP Address: ");
          Serial.println(WiFi.localIP());
        } else {
          Serial.println("\n[WARN] Failed to connect to saved WiFi.");
        }
      } else {
        Serial.println("[INFO] No saved WiFi credentials.");
      }
      
      // start mDNS service
      if (MDNS.begin(_hostname.c_str())) {
        MDNS.addService("http", "tcp", 80); // advertise web server on port 80
        Serial.println("[INFO] mDNS responder started.");
        Serial.print("[INFO] Access dashboard at: http://");
        Serial.print(_hostname);
        Serial.println(".local");
      } else {
        Serial.println("[ERROR] Error setting up mDNS responder!");
      }
    }
    
    // returns the device hostname
    String getHostname() {
        return _hostname;
    }
};