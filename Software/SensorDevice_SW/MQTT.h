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

    // callback function pointers
    DataRequestCallback _dataRequestCallback;
    ConfigCallback _configCallback;

    // mqtt topics
    String _dataTopic;
    String _reqTopic;
    String _confTopic;

    // internal reconnect logic
    void reconnect() {
        while (!_mqttClient.connected()) { // loop until we're reconnected
            Serial.print("Attempting MQTT connection...");
            if (*_p_ccu_address == "") { // if ccu address is not set
                Serial.println(" CCU address not set. Retrying in 5 seconds...");
                delay(5000); // wait and retry
                continue; // skip the rest of the loop
            }

            if (_mqttClient.connect(_device_id.c_str())) { // attempt to connect
                Serial.println("connected");
                _mqttClient.subscribe(_reqTopic.c_str()); // subscribe to request topic
                _mqttClient.subscribe(_confTopic.c_str()); // subscribe to config topic
                Serial.println("Subscribed to topics: ");
                Serial.println(_reqTopic);
                Serial.println(_confTopic);
            } else { // if connection failed
                Serial.print("failed, rc=");
                Serial.print(_mqttClient.state());
                Serial.println(" try again in 5 seconds");
                delay(5000); // wait 5 seconds before retrying
            }
        }
    }

    // internal callback that receives all messages and decides what to do
    void internalCallback(char* topic, byte* payload, unsigned int length) {
        String topicStr = String(topic); // convert topic to string
        
        if (_debug_enabled) { // use a local debug flag for clarity
            Serial.println("-----------");
            Serial.print("Message arrived on topic: ");
            Serial.println(topicStr);
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
    
    bool _debug_enabled = false; // internal debug flag

public:
    // constructor now takes a pointer to a PowerManager object
    MQTTClient(String* p_ccu_address, PowerManager* p_power_manager) 
        :   _p_ccu_address(p_ccu_address),
            _p_power_manager(p_power_manager),
            _dataRequestCallback(nullptr),
            _configCallback(nullptr),
            _mqttClient(_wifiClient)
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

    // set the function to be called when a data request arrives
    void onDataRequest(DataRequestCallback callback) {
        _dataRequestCallback = callback;
    }

    // set the function to be called when a config message arrives
    void onConfig(ConfigCallback callback) {
        _configCallback = callback;
    }
    
    // sets the mqtt server and port
    void begin() {
        if (*_p_ccu_address != "") { // if ccu address is available
            _mqttClient.setServer((*_p_ccu_address).c_str(), 1883); // set the mqtt server
        }
    }

    // must be called in the main loop()
    void loop() {
        if (*_p_ccu_address == "") return; // do nothing if ccu address is not set

        // if the server address has changed in the dashboard
        if (strcmp(_mqttClient.getServer(), (*_p_ccu_address).c_str()) != 0) {
            _mqttClient.setServer((*_p_ccu_address).c_str(), 1883); // update the server address
             if (_mqttClient.connected()) _mqttClient.disconnect(); // disconnect from the old server
        }

        if (!_mqttClient.connected()) { // if not connected
            reconnect(); // try to reconnect
        }
        _mqttClient.loop(); // process incoming messages and maintain connection
    }

    // publishes sensor data to the data topic
    void publishData(const String& jsonPayload) {
        if (_mqttClient.connected()) { // if connected
            if (_debug_enabled) {
                Serial.print("Publishing message to ");
                Serial.println(_dataTopic);
            }
            _mqttClient.publish(_dataTopic.c_str(), jsonPayload.c_str()); // publish the message
        } else {
            if (_debug_enabled) Serial.println("Cannot publish, MQTT client not connected.");
        }
    }

    bool isConnected() {
        return _mqttClient.connected(); // return the connection status
    }
    
    void setDebug(bool debug) {
        _debug_enabled = debug; // enable or disable debug prints
    }

    String getDeviceId() {
        return _device_id; // return the generated device id
    }
};