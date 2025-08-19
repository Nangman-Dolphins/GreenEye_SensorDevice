#pragma once // prevents multiple inclusion of the header file

enum PowerMode {
    ULTRA_LOW_POWER,
    LOW_POWER,
    NORMAL,
    HIGH_FREQ,
    ULTRA_HIGH_FREQ
};

class PowerManager {
private:
    PowerMode _currentMode;
    bool _nightModeEnabled;
    unsigned long _senseInterval;
    unsigned long _camInterval;

    void updateIntervals() {
        switch (_currentMode) {
            case ULTRA_LOW_POWER: _senseInterval = 2 * 3600; _camInterval = 4 * 3600; break;
            case LOW_POWER:       _senseInterval = 1 * 3600; _camInterval = 2 * 3600; break;
            case NORMAL:          _senseInterval = 30 * 60;  _camInterval = 1 * 3600; break;
            case HIGH_FREQ:       _senseInterval = 15 * 60;  _camInterval = 1 * 3600; break;
            case ULTRA_HIGH_FREQ: _senseInterval = 10 * 60;  _camInterval = 30 * 60;  break;
        }
    }

public:
    PowerManager() : 
        _currentMode(NORMAL),
        _nightModeEnabled(true)
    {
        updateIntervals();
    }

    void setMode(PowerMode newMode) { _currentMode = newMode; updateIntervals(); }
    void setMode(char modeChar) {
        if (modeChar == 'Z') setMode(ULTRA_LOW_POWER);
        else if (modeChar == 'L') setMode(LOW_POWER);
        else if (modeChar == 'M') setMode(NORMAL);
        else if (modeChar == 'H') setMode(HIGH_FREQ);
        else if (modeChar == 'U') setMode(ULTRA_HIGH_FREQ);
    }
    void setNightMode(bool enabled) { _nightModeEnabled = enabled; }
    unsigned long getSenseInterval() { return _senseInterval; }
    unsigned long getCamInterval() { return _camInterval; }
};