#pragma once // prevents multiple inclusion of the header file

#include <WiFi.h>        // for wifi status check
#include <NTPClient.h>   // for ntp time fetching
#include <WiFiUdp.h>     // for ntp communication

class TimeManager {
private:
    WiFiUDP _ntpUDP;              // udp client for ntp
    NTPClient _timeClient;        // ntp client object
    bool _time_synced = false;    // flag to check if time has been synced
    bool _debug_enabled = false;  // flag for controlling debug prints

public:
    // constructor initializes the ntp client
    TimeManager(bool debug = false)
        : _timeClient(_ntpUDP, "pool.ntp.org", 3600 * 9), // kst offset: 9 hours
          _debug_enabled(debug)
    {}

    // starts the ntp client
    void begin() {
        if (WiFi.status() == WL_CONNECTED) { // only begin if wifi is connected
            _timeClient.begin();
            if (_debug_enabled) { Serial.println("[TM] TimeManager initialized."); }
        }
    }

    // fetches the current time from the ntp server
    bool updateTime() {
        if (WiFi.status() != WL_CONNECTED) { // check for wifi connection
            if (_debug_enabled) { Serial.println("[TM][ERROR] Cannot update time, WiFi not connected."); }
            return false;
        }
        if (_timeClient.update()) { // attempt to update the time
            _time_synced = true;
            if (_debug_enabled) {
                Serial.print("[TM] Time updated successfully: ");
                Serial.println(getFormattedTime());
            }
            return true;
        } else {
            if (_debug_enabled) { Serial.println("[TM][WARN] Failed to update time from NTP server."); }
            return false;
        }
    }

    // returns true if the current time is between 21:00 and 05:59
    bool isNightTime() {
        if (!_time_synced) { // if time has not been synced yet
            if (_debug_enabled) { Serial.println("[TM][WARN] Time not synced, cannot determine if it's night."); }
            updateTime(); // try to update time again
            if (!_time_synced) return false; // if still not synced, assume it's not night to allow normal operation
        }
        int currentHour = _timeClient.getHours(); // get the current hour (0-23)
        // night is from 9 pm (21) to 6 am (before 6)
        return (currentHour >= 21 || currentHour < 6);
    }
    
    // calculates how many seconds until 6 am
    unsigned long getSecondsUntil6AM() {
        if (!_time_synced) {
            if (_debug_enabled) { Serial.println("[TM][WARN] Time not synced, returning default sleep interval."); }
            // return a reasonable default if time is not available to avoid infinite sleep
            return 3600; // 1 hour
        }

        int currentHour = _timeClient.getHours();
        int currentMinute = _timeClient.getMinutes();
        int currentSecond = _timeClient.getSeconds();

        // calculate total seconds from the beginning of the day
        long secondsPassedToday = currentHour * 3600L + currentMinute * 60L + currentSecond;

        // target time is 6:00 am, which is 6 * 3600 = 21600 seconds from midnight
        long targetSeconds = 6 * 3600L;

        long secondsToSleep;

        if (secondsPassedToday < targetSeconds) {
            // if current time is between midnight and 6 am (e.g., 2 am)
            // sleep until 6 am of the same day
            secondsToSleep = targetSeconds - secondsPassedToday;
        } else {
            // if current time is after 6 am (e.g., 9 pm)
            // sleep until 6 am of the *next* day
            long secondsInADay = 24 * 3600L;
            secondsToSleep = (secondsInADay - secondsPassedToday) + targetSeconds;
        }

        if (_debug_enabled) { Serial.printf("[TM] Seconds until 6 AM: %lu\n", (unsigned long)secondsToSleep); }
        // add a small buffer (e.g., 60 seconds) to ensure it wakes up after 6 am
        return (unsigned long)secondsToSleep + 60;
    }


    // returns the formatted time as a string
    String getFormattedTime() {
        if (!_time_synced) { return "Time not synced"; }
        return _timeClient.getFormattedTime();
    }
};