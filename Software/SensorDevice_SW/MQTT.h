#pragma once // prevents multiple inclusion of the header file

#include <WiFi.h>        // required for mac address
#include <WiFiClient.h>  // for tcp connection
#include <PubSubClient.h> // for mqtt communication
#include <ArduinoJson.h> // for json data handling
#include "PowerManager.h" // include power manager to control modes

// define function pointer types for specific events
using DataRequestCallback = void(*)(); // callback for when a data request arrives
using ConfigCallback = void(*)(JsonDocument& configDoc, PowerManager& pm); // callback for when a config message arrives

class MQTTClient {
private:
    WiFiClient _wifiClient;      // tcp client for mqtt
    PubSubClient _mqttClient;    // mqtt client object
    String* _p_ccu_address;      // pointer to the ccu address string
    String _device_id;           // this device's id (mac suffix)
    PowerManager* _p_power_manager; // pointer to the power manager object
    bool _debug_enabled = false; // flag for controlling debug prints

    DataRequestCallback _dataRequestCallback; // pointer to the data request callback function
    ConfigCallback _configCallback;           // pointer to the config callback function

    String _dataTopic;          // topic for publishing data
    String _reqTopic;           // topic for subscribing to requests
    String _confTopic;          // topic for subscribing to configs
    String _last_ccu_address;   // stores the last known ccu address

    // returns true on success, false on failure after retries
    bool reconnect() {
        byte retries = 5; // try to connect 5 times
        while (!_mqttClient.connected()) {
            if (retries == 0) { // if retries are exhausted
                if (_debug_enabled) { Serial.println(" [ERROR] Failed to connect to MQTT broker after multiple retries."); }
                return false; // give up and return failure
            }

            if (_debug_enabled) { Serial.printf("Attempting MQTT connection... (%d retries left)\n", retries); }
            
            if ((*_p_ccu_address).isEmpty()) { // if ccu address is not set
                if (_debug_enabled) { Serial.println(" CCU address not set. Retrying in 5 seconds..."); }
                delay(5000); // wait and retry
                retries--;
                continue;    // skip the rest of the loop
            }

            if (_mqttClient.connect(_device_id.c_str())) { // attempt to connect
                if (_debug_enabled) { Serial.println("connected"); }
                _mqttClient.subscribe(_reqTopic.c_str());    // subscribe to request topic
                _mqttClient.subscribe(_confTopic.c_str());   // subscribe to config topic
                return true; // connection successful
            } else { // if connection failed
                if (_debug_enabled) {
                    Serial.print("failed, rc=");
                    Serial.print(_mqttClient.state());
                    Serial.println(" try again in 5 seconds");
                }
                delay(5000); // wait 5 seconds before retrying
                retries--;
            }
        }
        return true; // already connected if loop is not entered
    }


    void internalCallback(char* topic, byte* payload, unsigned int length) { // receives all messages from broker
        String topicStr = String(topic); // convert topic to string
        
        if (_debug_enabled) { // print received message if debug is on
            String payloadStr;
            for (int i=0; i<length; i++) { payloadStr += (char)payload[i]; }
            Serial.println("-----------");
            Serial.print("Message arrived on topic: "); Serial.println(topicStr);
            Serial.print("Payload: "); Serial.println(payloadStr);
            Serial.println("-----------");
        }
        
        if (topicStr == _reqTopic) { // if it's a request topic
            if (_dataRequestCallback) { _dataRequestCallback(); } // trigger the data request callback
        } 
        else if (topicStr == _confTopic) { // if it's a config topic
            if (_configCallback && _p_power_manager) { // if callback and power manager are valid
                JsonDocument doc; // create a json document
                deserializeJson(doc, payload, length); // parse the payload
                _configCallback(doc, *_p_power_manager); // pass the parsed json and power manager to the callback
            }
        }
    }

public:
    MQTTClient(String* p_ccu_address, PowerManager* p_power_manager, bool debug) 
        : _p_ccu_address(p_ccu_address),          // store pointer to ccu address
          _p_power_manager(p_power_manager),      // store pointer to power manager
          _dataRequestCallback(nullptr),          // initialize callback to null
          _configCallback(nullptr),               // initialize callback to null
          _mqttClient(_wifiClient),               // initialize pubsubclient with wifi client
          _debug_enabled(debug)
    {
        String mac = WiFi.macAddress(); // get the mac address
        _device_id = mac.substring(12, 14) + mac.substring(15, 17); // get last 4 hex digits
        _device_id.toUpperCase(); // convert to uppercase

        // assign the topics based on the device id
        _dataTopic = "GreenEye/data/" + _device_id;
        _reqTopic = "GreenEye/req/" + _device_id;
        _confTopic = "GreenEye/conf/" + _device_id;

        // link the internal pubsubclient callback to our class method
        _mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
            this->internalCallback(topic, payload, length);
        });
    }

    void onDataRequest(DataRequestCallback callback) { // sets the function to be called when a data request arrives
        _dataRequestCallback = callback;
    }

    void onConfig(ConfigCallback callback) { // sets the function to be called when a config message arrives
        _configCallback = callback;
    }
    
    void begin() { // sets the mqtt server and port
        if (!(*_p_ccu_address).isEmpty()) { // if ccu address is available
            _mqttClient.setServer((*_p_ccu_address).c_str(), 1883); // set the mqtt server
            _last_ccu_address = *_p_ccu_address; // store it as the last known address
        }

        _mqttClient.setBufferSize(32786);
    }

    // returns true if connected, false if connection fails after retries
    bool loop() {
        if ((*_p_ccu_address).isEmpty()) return false; // do nothing if ccu address is not set

        if (*_p_ccu_address != _last_ccu_address) { // if the server address has changed in the dashboard
            if (_debug_enabled) { Serial.println("CCU address has changed. Reconfiguring MQTT client."); }
            _mqttClient.setServer((*_p_ccu_address).c_str(), 1883); // update the server address
            _last_ccu_address = *_p_ccu_address; // save the new address
            if (_mqttClient.connected()) { // if connected to the old server
                _mqttClient.disconnect(); // disconnect to force a reconnect to the new server
            }
        }

        if (!_mqttClient.connected()) { // if not connected
            if (!reconnect()) { // try to reconnect
                return false; // return failure if reconnect itself fails after retries
            }
        }
        _mqttClient.loop(); // process incoming messages and maintain connection
        return _mqttClient.connected();
    }


    void publishData(const String& jsonPayload) { // publishes sensor data to the data topic
        if (_mqttClient.connected()) { // if connected
            if (_debug_enabled) {
              Serial.print("Publishing message to ");
              Serial.println(_dataTopic);
            }
            _mqttClient.publish(_dataTopic.c_str(), jsonPayload.c_str()); // publish the message
        } else {
            if (_debug_enabled) { Serial.println("Cannot publish, MQTT client not connected."); }
        }
    }

    bool isConnected() { // returns the connection status
        return _mqttClient.connected();
    }
    
    void setDebug(bool debug) { // enables or disables debug prints
        _debug_enabled = debug;
    }

    String getDeviceId() { // returns the generated device id
        return _device_id;
    }
};