#pragma once // prevents multiple inclusion of the header file

#include <Wire.h> // for i2c communication
#include <cmath>  // for log() function

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

    // private helper to read from ads1015 with retries
    int16_t readADS1015(byte channel) {
        for (int i = 0; i < SENSOR_READ_RETRIES; i++) { // loop for retry attempts
            if (channel > 3) { return -1; } // ensure channel is valid
            uint16_t config = 0; // config variable
            config |= (1 << 15); // os: start a single conversion
            config |= ((4 + channel) << 12); // mux: set channel
            config |= (1 << 9);  // pga: gain 1, for +/- 4.096v range
            config |= (1 << 8);  // mode: single-shot mode
            config |= (4 << 5);  // dr: 1600 samples per second
            config |= 3;         // comp: comparator disabled
            
            Wire.beginTransmission(ADS1015_ADDRESS); // start transmission
            Wire.write(0x01); // point to config register
            Wire.write((config >> 8) & 0xFF); // write high byte
            Wire.write(config & 0xFF);   // write low byte
            Wire.endTransmission(); // end transmission
            delay(8); // wait for conversion
            
            Wire.beginTransmission(ADS1015_ADDRESS); // start transmission
            Wire.write(0x00); // point to conversion register
            Wire.endTransmission(); // end transmission
            
            Wire.requestFrom((uint8_t)ADS1015_ADDRESS, (size_t)2); // request 2 bytes
            if (Wire.available() == 2) { // if two bytes are received
                int16_t result = (Wire.read() << 8) | Wire.read(); // combine bytes
                return result >> 4; // return 12-bit result on success
            }
            delay(50); // short delay before retrying
        }
        if (_debug_enabled) Serial.printf("[SensorIO][ERROR] Failed to read ADC channel %d after %d retries.\n", channel, SENSOR_READ_RETRIES);
        return -1; // return -1 on failure to distinguish from 0
    }

    float getCalibratedEC(float adc_value) {
        const int NUM_CAL_POINTS = 9;
        const float adc_points[NUM_CAL_POINTS] = {
            310.4, 574.8, 657.9, 931.5, 1069.7, 1112.9, 1158.6, 1425.7, 1489.7
        };
        const float ec25_points[NUM_CAL_POINTS] = {
            23.9, 101.8, 167.3, 174.6, 426.5, 881.8, 1370.9, 2961.5, 5295.0
        };

        // 다점 선형 보간법 (Python 코드와 동일)
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

    void initI2C() {
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); // start i2c with specific pins
    }

    void endI2C() {
        Wire.end(); // releases the i2c bus and pins
    }
    
    void initTempHumiSensor() {
        Wire.beginTransmission(AHT20_ADDR); // start transmission to aht20
        Wire.write(0xBE); // send initialization command
        Wire.endTransmission(); // end transmission
        delay(100); // wait after initialization
    }

    void initSoilMoistureSensor() {
        pinMode(SOIL_MOISTURE_EN_PIN, OUTPUT); // set enable pin as output
        digitalWrite(SOIL_MOISTURE_EN_PIN, LOW); // start with sensor off
    }

    void initECSensor() {
        pinMode(EC_PIN1, INPUT); // set ec pins to input to keep them off
        pinMode(EC_PIN2, INPUT);
    }
    
    bool begin() {
        initI2C(); // initialize the i2c bus
        initTempHumiSensor(); // initialize the aht20 sensor
        initSoilMoistureSensor(); // set up the soil moisture pin
        initECSensor(); // set up the ec pins
        if (_debug_enabled) Serial.println("[SensorIO] Initialized."); // log initialization
        return true; // return success
    }

    // --- Individual Measurement Functions with Averaging, Raw Mode and Retries ---

    void readBatteryLevel(int rawMode) {
        if (!_p_battery_level) return; // check for valid pointer
        if (_debug_enabled) Serial.println("[SensorIO] Reading Battery Level...");
        long total = 0; // use long for accumulator
        int valid_readings = 0; // count valid readings
        for (int i = 0; i < SENSOR_AVG_COUNT; i++) { // loop 10 times
            int16_t adcValue = readADS1015(2); // read from adc channel 2 for bat_level
            if (adcValue >= 0) { // process valid readings (including 0)
                total += adcValue; // add to total
                valid_readings++; // increment count
            }
            delay(10); // short delay between readings
        }

        if (valid_readings == 0) { 
            if (_debug_enabled) Serial.println("[SensorIO][ERROR] Failed to get any valid battery readings.");
            *_p_battery_level=0; 
            return; 
        }
        
        int16_t avgAdcValue = total / valid_readings; // calculate average adc value
        if (_debug_enabled) Serial.printf("  [DEBUG] Battery Average ADC: %d\n", avgAdcValue);

        if (rawMode == 1) { // if raw mode is requested
            *_p_battery_level = avgAdcValue; // store raw average
        } else { // if calculated mode is requested (default)
            float dividerVoltage = avgAdcValue * 0.002; // convert raw adc to voltage at divider
            float batteryVoltage = dividerVoltage * 2.0; // calculate actual battery voltage
            float clamped_voltage = constrain(batteryVoltage, 3.0, 4.2); // clamp voltage to 3.0v - 4.2v
            *_p_battery_level = map(clamped_voltage * 100, 300, 420, 0, 100); // map voltage to percentage
            if (_debug_enabled) Serial.printf("  [DEBUG] Battery Voltage: %.2fV, Percentage: %d%%\n", batteryVoltage, *_p_battery_level);
        }
    }

    void readAmbientTempHumi() {
        if (!_p_temp_ambient || !_p_humidity) return; // check for valid pointers
        if (_debug_enabled) Serial.println("[SensorIO] Reading Ambient Temp/Humi...");
        float temp_total = 0; // accumulator for temperature
        float humi_total = 0; // accumulator for humidity
        int valid_readings = 0; // count of successful readings
        for (int i = 0; i < SENSOR_AVG_COUNT; i++) { // loop 10 times
            bool success = false; // flag for successful read
            for (int retry = 0; retry < SENSOR_READ_RETRIES; retry++) { // inner loop for retries
                Wire.beginTransmission(AHT20_ADDR); // start transmission
                Wire.write(0xAC); // send measurement command
                Wire.write(0x33);
                Wire.write(0x00);
                Wire.endTransmission();
                delay(80); // wait for measurement
                byte data[7]; // create buffer
                if (Wire.requestFrom((uint8_t)AHT20_ADDR, (uint8_t)7) == 7) { // if 7 bytes are received
                    for (int j=0; j < 7; j++) { data[j] = Wire.read(); } // read data into buffer
                    unsigned long rawHumidity = ((unsigned long)data[1] << 16 | (unsigned long)data[2] << 8 | data[3]) >> 4; // parse raw humidity
                    unsigned long rawTemp = (((unsigned long)data[3] & 0x0F) << 16 | (unsigned long)data[4] << 8 | data[5]); // parse raw temperature
                    float temp = ( ( (float)rawTemp / 1048576.0 ) * 200.0 ) - 50.0; // calculate final temperature
                    float humi = ( (float)rawHumidity / 1048576.0 ) * 100.0; // calculate final humidity
                    temp_total += temp; // add to total
                    humi_total += humi; // add to total
                    valid_readings++;   // increment count
                    success = true; // mark as successful
                    break; // successfully read, so exit retry loop
                }
                delay(50); // wait before retrying
            }
            if (!success && i == SENSOR_AVG_COUNT - 1) { // if all retries failed on the last attempt
                 if (_debug_enabled) Serial.println("[SensorIO][ERROR] Failed to read from AHT20 sensor!");
                *_p_temp_ambient = -99.9; // set error value
                *_p_humidity = -99.9;     // set error value
                return;
            }
            delay(50); // wait before next measurement in the averaging loop
        }
        
        if (valid_readings > 0) {
            *_p_temp_ambient = temp_total / valid_readings; // calculate average temp
            *_p_humidity = humi_total / valid_readings; // calculate average humidity
            if (_debug_enabled) Serial.printf("  [DEBUG] Avg Temp: %.2f C, Avg Humi: %.2f %%\n", *_p_temp_ambient, *_p_humidity);
        }
    }

    void readLight() {
        if (!_p_light) return; // check for valid pointer
        if (_debug_enabled) Serial.println("[SensorIO] Reading Light...");
        float total_lux = 0; // accumulator for lux
        int valid_readings = 0; // count of successful readings
        for (int i = 0; i < SENSOR_AVG_COUNT; i++) { // loop 10 times
            bool success = false; // flag for successful read
            for (int retry = 0; retry < SENSOR_READ_RETRIES; retry++) { // inner loop for retries
                Wire.beginTransmission(GY302_ADDR); // start transmission
                Wire.write(0x20); // send one-time high-res command
                Wire.endTransmission();
                delay(130); // wait for measurement
                if (Wire.requestFrom((uint8_t)GY302_ADDR, (uint8_t)2) == 2) { // if 2 bytes are received
                    byte highByte = Wire.read(); // read high byte
                    byte lowByte = Wire.read(); // read low byte
                    unsigned int rawValue = (highByte << 8) | lowByte; // combine bytes
                    float lux = rawValue / 1.2; // calculate lux
                    total_lux += lux; // add to total
                    valid_readings++; // increment count
                    success = true; // mark as successful
                    break; // successfully read, so exit retry loop
                }
                delay(50); // wait before retrying
            }
            if (!success && i == SENSOR_AVG_COUNT - 1) {
                if (_debug_enabled) Serial.println("[SensorIO][ERROR] Failed to read from GY-302 sensor.");
                *_p_light = -1; // set error value
                return;
            }
            delay(50); // wait before next measurement
        }

        if (valid_readings > 0) {
            *_p_light = total_lux / valid_readings; // calculate average lux
            if (_debug_enabled) Serial.printf("  [DEBUG] Avg Light: %.2f lux\n", *_p_light);
        }
    }

    void readSoilTemp(int rawMode) {
        if (!_p_temp_soil) return;
        if (_debug_enabled) Serial.printf("[SensorIO] Reading Soil Temperature (rawMode=%d)...\n", rawMode);
        long total = 0;
        int valid_readings = 0;
        for (int i = 0; i < SENSOR_AVG_COUNT; i++) {
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
        
        float total_avg_reading = 0.0f; // float 타입으로 변경
        int valid_readings = 0;

        pinMode(EC_PIN1, OUTPUT);
        pinMode(EC_PIN2, OUTPUT);
        delay(10);

        for (int i = 0; i < SENSOR_AVG_COUNT; i++) {
            digitalWrite(EC_PIN1, HIGH); digitalWrite(EC_PIN2, LOW);
            delay(15);
            int reading1 = readADS1015(3);

            digitalWrite(EC_PIN1, LOW); digitalWrite(EC_PIN2, LOW);
            delay(15);
          
            digitalWrite(EC_PIN1, LOW); digitalWrite(EC_PIN2, HIGH);
            delay(15);
            int reading2 = readADS1015(3);
            
            if (reading1 >= 0 && reading2 >= 0) {
                // 모든 계산을 float으로 수행하여 정밀도 유지
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
        
        // --- CALIBRATION AND TEMPERATURE COMPENSATION LOGIC ---
        
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

    void readAllSensors(int rawMode = 0) {
        if (_debug_enabled) { Serial.println("\n[SensorIO] --- Starting Full Sensor Read Cycle ---"); }
        readAmbientTempHumi();
        delay(1000);
        readLight();
        delay(1000);
        readSoilTemp(rawMode);
        delay(1000);
        readSoilEC(rawMode);
        delay(1000);
        readBatteryLevel(rawMode);
        delay(1000);
        readSoilMoisture(rawMode);
        if (_debug_enabled) { Serial.println("[SensorIO] --- Sensor Read Cycle Complete ---"); }
    }
};