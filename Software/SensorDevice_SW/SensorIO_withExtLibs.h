#pragma once // prevents multiple inclusion of the header file

#include <Wire.h> // still needed for the i2c bus itself
#include <cmath>  // for log() function

// --- include the external sensor libraries ---
#include <Adafruit_ADS1X15.h> // for the ads1015 adc
#define sensor_t adafruit_sensor_t
#include <Adafruit_AHTX0.h>
#undef sensor_t // remove the temporary name change
#include <BH1750.h>           // for the gy-302 (bh1750) light sensor

#define SENSOR_READ_RETRIES 5 // macro for sensor read retry count
#define SENSOR_AVG_COUNT 10   // macro for sensor averaging count

class SensorIO {
private:
    // i2c addresses
    const byte ADS1015_ADDRESS = 0x48; // i2c address for ads1015
    const byte AHT20_ADDR = 0x38;      // i2c address for aht20
    const byte GY302_ADDR = 0x23;      // i2c address for gy-302

    // pin definitions
    const int I2C_SDA_PIN = 14; // i2c sda pin
    const int I2C_SCL_PIN = 2;  // i2c scl pin
    const int SOIL_MOISTURE_EN_PIN = 15; // soil moisture sensor enable pin
    const int EC_PIN1 = 12; // ec probe pin 1
    const int EC_PIN2 = 13; // ec probe pin 2

    // pointers to external data variables
    int* _p_battery_level;
    float* _p_temp_ambient;
    float* _p_humidity;
    float* _p_light;
    float* _p_temp_soil;
    float* _p_moisture;
    float* _p_ec;
    bool _debug_enabled;

    // --- library objects ---
    Adafruit_ADS1015 ads; // ads1015 adc object
    Adafruit_AHTX0 aht;   // aht20 temp/humi object
    BH1750 lightMeter;    // gy-302 light sensor object

    // private helper to read from ads1015 (now using the library)
    int16_t readADS1015(byte channel) {
        for (int i = 0; i < SENSOR_READ_RETRIES; i++) {
            if (channel > 3) { return -1; } // ensure channel is valid
            // the library handles configuration and reading in one step
            int16_t result = ads.readADC_SingleEnded(channel);
            // the library is quite stable, but we keep the retry logic for consistency
            if (result != 0x8000) { // check for library error code (though unlikely)
                return result;
            }
            delay(50); // short delay before retrying
        }
        if (_debug_enabled) Serial.printf("[SensorIO][ERROR] Failed to read ADC channel %d after %d retries.\n", channel, SENSOR_READ_RETRIES);
        return -1; // return -1 on failure
    }

    // this custom calibration function remains unchanged
    float getCalibratedEC(float adc_value) {
        const int NUM_CAL_POINTS = 9;
        const float adc_points[NUM_CAL_POINTS] = {
            310.4, 574.8, 657.9, 931.5, 1069.7, 1112.9, 1158.6, 1425.7, 1489.7
        };
        const float ec25_points[NUM_CAL_POINTS] = {
            23.9, 101.8, 167.3, 174.6, 426.5, 881.8, 1370.9, 2961.5, 5295.0
        };

        for (int i = 0; i < NUM_CAL_POINTS - 1; i++) {
            if (adc_value >= adc_points[i] && adc_value <= adc_points[i+1]) {
                float x1 = adc_points[i];
                float y1 = ec25_points[i];
                float x2 = adc_points[i+1];
                float y2 = ec25_points[i+1];
                if (x1 == x2) return y1;
                return y1 + (adc_value - x1) * (y2 - y1) / (x2 - x1);
            }
        }

        if (adc_value < adc_points[0]) return ec25_points[0];
        else return ec25_points[NUM_CAL_POINTS - 1];
    }

public:
    // constructor is identical to the original
    SensorIO(
        int* p_battery_level,
        float* p_temp_ambient, float* p_humidity, float* p_light,
        float* p_temp_soil, float* p_moisture, float* p_ec,
        bool debug = false
    ) : _p_battery_level(p_battery_level),
        _p_temp_ambient(p_temp_ambient),
        _p_humidity(p_humidity),
        _p_light(p_light),
        _p_temp_soil(p_temp_soil),
        _p_moisture(p_moisture),
        _p_ec(p_ec),
        _debug_enabled(debug)
    {}

    void endI2C() {
        Wire.end(); // releases the i2c bus and pins
    }
    
    // pin mode setups for non-i2c sensors remain
    void initSoilMoistureSensor() {
        pinMode(SOIL_MOISTURE_EN_PIN, OUTPUT);
        digitalWrite(SOIL_MOISTURE_EN_PIN, LOW);
    }

    void initECSensor() {
        pinMode(EC_PIN1, INPUT);
        pinMode(EC_PIN2, INPUT);
    }
    
    bool begin() {
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); // start i2c with specific pins

        // initialize sensors using their libraries
        if (!ads.begin(ADS1015_ADDRESS)) {
            if (_debug_enabled) Serial.println("[SensorIO][ERROR] Failed to find ADS1015 chip");
            return false;
        }
        ads.setGain(GAIN_TWOTHIRDS); // gain 2/3 for +/- 6.144v range (matches original config)

        if (!aht.begin()) {
            if (_debug_enabled) Serial.println("[SensorIO][ERROR] Failed to find AHT20 chip");
            return false;
        }

        if (!lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE, GY302_ADDR)) {
            if (_debug_enabled) Serial.println("[SensorIO][ERROR] Failed to find GY-302 chip");
            return false;
        }
        
        // initialize other non-i2c components
        initSoilMoistureSensor();
        initECSensor();
        
        if (_debug_enabled) Serial.println("[SensorIO] Initialized with external libraries.");
        return true;
    }

    // --- Individual Measurement Functions (Public Interface Unchanged) ---

    void readBatteryLevel(int rawMode) {
        if (!_p_battery_level) return;
        if (_debug_enabled) Serial.println("[SensorIO] Reading Battery Level...");
        long total = 0;
        int valid_readings = 0;
        for (int i = 0; i < SENSOR_AVG_COUNT; i++) {
            // using the library to read adc channel 2
            int16_t adcValue = readADS1015(2);
            if (adcValue >= 0) {
                total += adcValue;
                valid_readings++;
            }
            delay(10);
        }

        if (valid_readings == 0) { 
            if (_debug_enabled) Serial.println("[SensorIO][ERROR] Failed to get any valid battery readings.");
            *_p_battery_level = 0; 
            return; 
        }
        
        int16_t avgAdcValue = total / valid_readings;
        if (_debug_enabled) Serial.printf("  [DEBUG] Battery Average ADC: %d\n", avgAdcValue);

        // calculation logic remains identical
        if (rawMode == 1) {
            *_p_battery_level = avgAdcValue;
        } else {
            float dividerVoltage = avgAdcValue * 0.1875;
            float batteryVoltage = dividerVoltage * 2.0;
            float clamped_voltage = constrain(batteryVoltage, 3.0, 4.2);
            *_p_battery_level = map(clamped_voltage * 100, 300, 420, 0, 100);
            if (_debug_enabled) Serial.printf("  [DEBUG] Battery Voltage: %.2fV, Percentage: %d%%\n", batteryVoltage, *_p_battery_level);
        }
    }

    void readAmbientTempHumi() {
        if (!_p_temp_ambient || !_p_humidity) return;
        if (_debug_enabled) Serial.println("[SensorIO] Reading Ambient Temp/Humi...");
        float temp_total = 0;
        float humi_total = 0;
        int valid_readings = 0;
        for (int i = 0; i < SENSOR_AVG_COUNT; i++) {
            bool success = false;
            for (int retry = 0; retry < SENSOR_READ_RETRIES; retry++) {
                // create sensor event objects to store readings
                sensors_event_t humidity_event, temp_event;
                // use library to get sensor readings
                if (aht.getEvent(&humidity_event, &temp_event)) {
                    temp_total += temp_event.temperature;
                    humi_total += humidity_event.relative_humidity;
                    valid_readings++;
                    success = true;
                    break; // exit retry loop on success
                }
                delay(50); // wait before retrying
            }
            if (!success && i == SENSOR_AVG_COUNT - 1) {
                if (_debug_enabled) Serial.println("[SensorIO][ERROR] Failed to read from AHT20 sensor!");
                *_p_temp_ambient = -99.9;
                *_p_humidity = -99.9;
                return;
            }
            delay(50);
        }
        
        if (valid_readings > 0) {
            *_p_temp_ambient = temp_total / valid_readings;
            *_p_humidity = humi_total / valid_readings;
            if (_debug_enabled) Serial.printf("  [DEBUG] Avg Temp: %.2f C, Avg Humi: %.2f %%\n", *_p_temp_ambient, *_p_humidity);
        }
    }

    void readLight() {
        if (!_p_light) return;
        if (_debug_enabled) Serial.println("[SensorIO] Reading Light...");
        float total_lux = 0;
        int valid_readings = 0;
        for (int i = 0; i < SENSOR_AVG_COUNT; i++) {
            bool success = false;
            for (int retry = 0; retry < SENSOR_READ_RETRIES; retry++) {
                // use library to read light level, it handles delays
                float lux = lightMeter.readLightLevel();
                // readLightLevel returns < 0 on error
                if (lux >= 0) {
                    total_lux += lux;
                    valid_readings++;
                    success = true;
                    break; // exit retry loop on success
                }
                delay(50); // wait before retrying
            }
            if (!success && i == SENSOR_AVG_COUNT - 1) {
                if (_debug_enabled) Serial.println("[SensorIO][ERROR] Failed to read from GY-302 sensor.");
                *_p_light = -1;
                return;
            }
            delay(50);
        }

        if (valid_readings > 0) {
            *_p_light = total_lux / valid_readings;
            if (_debug_enabled) Serial.printf("  [DEBUG] Avg Light: %.2f lux\n", *_p_light);
        }
    }

    void readSoilTemp(int rawMode) {
        if (!_p_temp_soil) return;
        if (_debug_enabled) Serial.printf("[SensorIO] Reading Soil Temperature (rawMode=%d)...\n", rawMode);
        long total = 0;
        int valid_readings = 0;
        for (int i = 0; i < SENSOR_AVG_COUNT; i++) {
            // using library to read adc channel 0
            int16_t adcValue = readADS1015(0);
            if (_debug_enabled) { Serial.printf("  [RAW] Reading #%d: ADC=%d\n", i + 1, adcValue); }
            if (adcValue >= 0) {
                total += adcValue;
                valid_readings++;
            }
            delay(10);
        }
        
        if (valid_readings == 0) { 
            if (_debug_enabled) Serial.println("[SensorIO][ERROR] Failed to get any valid soil temp readings."); 
            *_p_temp_soil = -99.9; 
            return; 
        }
        
        int16_t avgAdcValue = total / valid_readings;
        if (_debug_enabled) Serial.printf("  [DEBUG] Soil Temp Average ADC: %d\n", avgAdcValue);
        
        // calculation logic remains identical
        if (rawMode == 1) {
            *_p_temp_soil = avgAdcValue;
        } else {
            float voltage = avgAdcValue * 0.002;
            if (voltage <= 0) { *_p_temp_soil = -99.9; return; }
            float resistance = 10000.0 * ( (3.3 - voltage) / voltage );
            float steinhart = log(resistance / 10000.0) / 3950.0 + (1.0 / (25.0 + 273.15));
            *_p_temp_soil = (1.0 / steinhart) - 273.15;
            if (_debug_enabled) Serial.printf("  [DEBUG] Soil Resistance: %.2f Ohms, Temp: %.2f C\n", resistance, *_p_temp_soil);
        }
    }

    void readSoilMoisture(int rawMode) {
        if (!_p_moisture) return;
        if (_debug_enabled) Serial.println("[SensorIO] Reading Soil Moisture...");
        digitalWrite(SOIL_MOISTURE_EN_PIN, HIGH);
        delay(500);

        long total = 0;
        int valid_readings = 0;
        for (int i = 0; i < SENSOR_AVG_COUNT; i++) {
            // using library to read adc channel 1
            int16_t adcValue = readADS1015(1);
            if (adcValue >= 0) {
                total += adcValue;
                valid_readings++;
            }
            delay(10);
        }
        
        digitalWrite(SOIL_MOISTURE_EN_PIN, LOW);
        
        if (valid_readings == 0) { 
            if (_debug_enabled) Serial.println("[SensorIO][ERROR] Failed to get any valid soil moisture readings."); 
            *_p_moisture = 0; 
            return; 
        }
        
        int16_t avgAdcValue = total / valid_readings;
        if (_debug_enabled) Serial.printf("  [DEBUG] Soil Moisture Average ADC: %d\n", avgAdcValue);

        // calculation logic remains identical
        if (rawMode == 1) {
            *_p_moisture = avgAdcValue;
        } else {
            *_p_moisture = map(avgAdcValue, 0, 2047, 100, 0);
            if (_debug_enabled) Serial.printf("  [DEBUG] Soil Moisture Percentage: %.2f %%\n", *_p_moisture);
        }
    }

    void readSoilEC(int rawMode) {
        if (!_p_ec || !_p_temp_soil) return;
        if (_debug_enabled) Serial.println("[SensorIO] Reading Soil EC...");
        
        float total_avg_reading = 0.0f;
        int valid_readings = 0;

        pinMode(EC_PIN1, OUTPUT);
        pinMode(EC_PIN2, OUTPUT);
        delay(10);

        for (int i = 0; i < SENSOR_AVG_COUNT; i++) {
            // pin toggling logic remains identical
            digitalWrite(EC_PIN1, HIGH); digitalWrite(EC_PIN2, LOW);
            delay(15);
            // using library to read adc channel 3
            int reading1 = readADS1015(3);

            digitalWrite(EC_PIN1, LOW); digitalWrite(EC_PIN2, LOW);
            delay(15);
          
            digitalWrite(EC_PIN1, LOW); digitalWrite(EC_PIN2, HIGH);
            delay(15);
            // using library to read adc channel 3 again
            int reading2 = readADS1015(3);
            
            if (reading1 >= 0 && reading2 >= 0) {
                total_avg_reading += (reading1 + (2047.0f - reading2)) / 2.0f;
                valid_readings++;
            }
            delay(10);
        }

        digitalWrite(EC_PIN1, LOW); digitalWrite(EC_PIN2, LOW);
        pinMode(EC_PIN1, INPUT); pinMode(EC_PIN2, INPUT);

        if (valid_readings == 0) { 
            if (_debug_enabled) Serial.println("[SensorIO][ERROR] Failed to get any valid EC readings."); 
            *_p_ec = 0; 
            return; 
        }

        float final_avg_reading = total_avg_reading / valid_readings;
        if (_debug_enabled) Serial.printf("  [DEBUG] Soil EC Average ADC: %.2f\n", final_avg_reading);

        if (rawMode == 1) {
            *_p_ec = final_avg_reading;
            return;
        }
        
        // calibration and temp compensation logic remains identical
        float calibrated_ec25 = getCalibratedEC(final_avg_reading);
        if (_debug_enabled) Serial.printf("  [DEBUG] Calibrated EC25: %.2f uS/cm\n", calibrated_ec25);

        const float TEMP_COEFF = 0.02;
        float current_soil_temp = *_p_temp_soil;

        if (current_soil_temp < -20 || current_soil_temp > 60) {
            *_p_ec = calibrated_ec25;
            if (_debug_enabled) Serial.println("  [WARN] Invalid soil temp, returning uncompensated EC.");
        } else {
            float compensated_ec = calibrated_ec25 * (1.0f + TEMP_COEFF * (current_soil_temp - 25.0f));
            *_p_ec = compensated_ec;
            if (_debug_enabled) Serial.printf("  [DEBUG] Final EC at %.1f C: %.2f uS/cm\n", current_soil_temp, *_p_ec);
        }
    }

    // this function remains identical
    void readAllSensors(int rawMode = 0) {
        if (_debug_enabled) { Serial.println("\n[SensorIO] --- Starting Full Sensor Read Cycle ---"); }
        readAmbientTempHumi();
        delay(50);
        readLight();
        delay(50);
        readSoilTemp(rawMode);
        delay(50);
        readSoilEC(rawMode);
        delay(50);
        readBatteryLevel(rawMode);
        delay(50);
        readSoilMoisture(rawMode);
        if (_debug_enabled) { Serial.println("[SensorIO] --- Sensor Read Cycle Complete ---"); }
    }
};