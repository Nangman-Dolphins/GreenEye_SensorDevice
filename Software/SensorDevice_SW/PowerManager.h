// PowerManager.h

#pragma once // prevents multiple inclusion of the header file

#include <Preferences.h> // for non-volatile storage
#include "driver/rtc_io.h"  // added: include for rtc gpio functions
#include <Arduino.h>        // added: include for serial and esp functions
#include <Wire.h>           // added: include for i2c control

enum PowerMode { // defines the available power modes
    ULTRA_LOW_POWER, // z
    LOW_POWER,       // l
    NORMAL,          // m
    HIGH_FREQ,       // h
    ULTRA_HIGH_FREQ, // u
    DEBUGGING        // d // ADDED: Debugging mode enum
};

class PowerManager {
private:
    PowerMode _currentMode;     // holds the current power mode
    bool _nightModeEnabled;     // flag for night deep sleep mode
    unsigned long _senseInterval; // interval for sensor reading in seconds
    unsigned long _camInterval;   // interval for camera capture in seconds
    Preferences _preferences;   // non-volatile storage handler
    bool _debug_enabled;        // flag for debug mode

    void updateIntervals() { // updates the intervals based on the current mode
        switch (_currentMode) {
            case ULTRA_LOW_POWER: _senseInterval = 2 * 3600; _camInterval = 4 * 3600; break;
            case LOW_POWER:       _senseInterval = 1 * 3600; _camInterval = 2 * 3600; break;
            case NORMAL:          _senseInterval = 30 * 60;  _camInterval = 1 * 3600; break;
            case HIGH_FREQ:       _senseInterval = 20 * 60;  _camInterval = 1 * 3600; break;
            case ULTRA_HIGH_FREQ: _senseInterval = 20 * 60;  _camInterval = 40 * 60;  break;
            // ADDED: Interval settings for the new debugging mode
            case DEBUGGING:       _senseInterval = 30;       _camInterval = 60;       break;
        }
        if (_debug_enabled) { // check if debug mode is on
            Serial.printf("[PM] Mode updated. Sense interval: %lu s, Cam interval: %lu s\n", _senseInterval, _camInterval);
        }
    }

public:
    PowerManager(bool debug = false) : 
        _currentMode(NORMAL),        // set default power mode
        _nightModeEnabled(true),     // enable night mode by default
        _debug_enabled(debug)        // set debug mode from argument
    {
        updateIntervals(); // set initial intervals for the default mode
    }

    void begin() { // loads saved settings from non-volatile storage
        _preferences.begin("power-mgmt", false); // initialize preferences with a namespace
        char savedMode = _preferences.getChar("pwr_mode", 'M'); // load saved mode, default to 'm'
        _nightModeEnabled = _preferences.getBool("nht_mode", true); // load night mode setting, default to true
        setMode(savedMode); // apply the loaded or default mode
        if (_debug_enabled) { Serial.println("[PM] PowerManager initialized."); }
    }

    void setMode(PowerMode newMode) { // sets a new power mode
        _currentMode = newMode; // update the internal mode
        if (_debug_enabled) { Serial.print("[PM] Set Mode to "); Serial.println(getModeString()); }
        updateIntervals(); // update the timing intervals
    }

    void setMode(char modeChar) { // sets a new power mode using a character
        PowerMode newMode = _currentMode; // default to current mode
        if (modeChar == 'Z') newMode = ULTRA_LOW_POWER;
        else if (modeChar == 'L') newMode = LOW_POWER;
        else if (modeChar == 'M') newMode = NORMAL;
        else if (modeChar == 'H') newMode = HIGH_FREQ;
        else if (modeChar == 'U') newMode = ULTRA_HIGH_FREQ;
        else if (modeChar == 'D') newMode = DEBUGGING; // ADDED: Set mode for 'D'
        
        if (newMode != _currentMode) { // if the mode actually changed
            setMode(newMode); // call the main setMode function
            _preferences.putChar("pwr_mode", modeChar); // save the new mode to storage
            if (_debug_enabled) { Serial.printf("[PM] Power mode set to '%c' and saved.\n", modeChar); }
        }
    }
    
    void setNightMode(bool enabled) { // enables or disables night mode
        _nightModeEnabled = enabled; // update the internal flag
        _preferences.putBool("nht_mode", enabled); // save the setting to storage
        if (_debug_enabled) { Serial.printf("[PM] Night mode set to: %s and saved.\n", enabled ? "ON" : "OFF"); }
    }
    
    String getModeString() {
        switch (_currentMode) {
            case ULTRA_LOW_POWER: return "Ultra Low Power";
            case LOW_POWER:       return "Low Power";
            case NORMAL:          return "Normal";
            case HIGH_FREQ:       return "High Freq";
            case ULTRA_HIGH_FREQ: return "Ultra High Freq";
            case DEBUGGING:       return "Debug Mode";
            default:              return "Unknown";
        }
    }

    // ADDED: getter for the current mode enum
    PowerMode getCurrentMode() { return _currentMode; }

    unsigned long getSenseInterval() { return _senseInterval; } // returns the current sensor interval
    unsigned long getCamInterval() { return _camInterval; }   // returns the current camera interval
    bool isNightModeEnabled() { return _nightModeEnabled; }    // returns if night mode is enabled

    // added: a method to check if camera data should be sent
    bool shouldSendCameraData(int currentBootCount) {
        // check to prevent division by zero
        if (_camInterval > 0 && _senseInterval > 0) {
            int sense_cycles_per_cam = _camInterval / _senseInterval;
            // ensure it makes sense to check the modulo
            if (sense_cycles_per_cam > 0) {
                 return (currentBootCount % sense_cycles_per_cam == 0);
            }
        }
        // return false if intervals are not set correctly
        return false;
    }

    // added: a method to handle deep sleep logic
    void enterDeepSleep(unsigned long sleep_seconds) {
        if (_debug_enabled) {
            Serial.print("[PM] Entering deep sleep for ");
            Serial.print(sleep_seconds);
            Serial.println(" seconds.");
        }
        
        // moved the deep sleep hardware control logic here
        Wire.end(); // disable i2c bus before sleep
        
        // setup wakeup pin
        rtc_gpio_pullup_en(GPIO_NUM_2);
        rtc_gpio_pulldown_dis(GPIO_NUM_2);
        
        delay(100); // short delay for stability

        // enable wakeup sources
        esp_sleep_enable_timer_wakeup(sleep_seconds * 1000000ULL);
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_2, 0); // wakeup on low level

        // start deep sleep
        esp_deep_sleep_start();
    }
};