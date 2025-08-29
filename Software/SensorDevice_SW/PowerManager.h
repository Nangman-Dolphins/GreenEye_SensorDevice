// PowerManager.h

#pragma once // prevents multiple inclusion of the header file

#include <Preferences.h> // for non-volatile storage

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

    // ADDED: Function to get the current power mode as a string for display
    String getModeString() {
        switch (_currentMode) {
            case ULTRA_LOW_POWER: return "초저전력";
            case LOW_POWER:       return "저전력";
            case NORMAL:          return "일반";
            case HIGH_FREQ:       return "고빈도";
            case ULTRA_HIGH_FREQ: return "초고빈도";
            case DEBUGGING:       return "디버그 모드";
            default:              return "Unknown";
        }
    }

    unsigned long getSenseInterval() { return _senseInterval; } // returns the current sensor interval
    unsigned long getCamInterval() { return _camInterval; }   // returns the current camera interval
    bool isNightModeEnabled() { return _nightModeEnabled; }    // returns if night mode is enabled
};