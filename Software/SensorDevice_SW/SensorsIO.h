#pragma once // prevents multiple inclusion of the header file

#include <Wire.h> // for i2c communication
#include <cmath>  // for log() function

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

    // private helper to read from ads1015, from your code
    int16_t readADS1015(byte channel) {
        if (channel > 3) { return 0; } // ensure channel is valid
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
        delay(2); // wait for conversion
        
        Wire.beginTransmission(ADS1015_ADDRESS); // start transmission
        Wire.write(0x00); // point to conversion register
        Wire.endTransmission(); // end transmission
        
        Wire.requestFrom((uint8_t)ADS1015_ADDRESS, (uint8_t)2); // request 2 bytes
        if (Wire.available() == 2) { // if two bytes are received
            int16_t result = (Wire.read() << 8) | Wire.read(); // combine bytes
            return result >> 4; // return 12-bit result
        }
        return 0; // return 0 on failure
    }

public:
    SensorIO(
        int* p_battery_level,
        float* p_temp_ambient, float* p_humidity, float* p_light,
        float* p_temp_soil, float* p_moisture, float* p_ec
    ) : _p_battery_level(p_battery_level),
        _p_temp_ambient(p_temp_ambient),
        _p_humidity(p_humidity),
        _p_light(p_light),
        _p_temp_soil(p_temp_soil),
        _p_moisture(p_moisture),
        _p_ec(p_ec)
    {}

    // --- Individual Initializers ---
    
    void initI2C() {
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); // start i2c with specific pins
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
    
    // --- Main Initializer ---
    
    bool begin() {
        initI2C(); // initialize the i2c bus
        initTempHumiSensor(); // initialize the aht20 sensor
        initSoilMoistureSensor(); // set up the soil moisture pin
        initECSensor(); // set up the ec pins
        Serial.println("SensorIO initialized."); // log initialization
        return true; // return success
    }

    // --- Individual Measurement Functions ---

    void readBatteryLevel() {
        if (!_p_battery_level) return; // check for valid pointer
        int16_t adcValue = readADS1015(2); // read from adc channel 2 for bat_level
        float dividerVoltage = adcValue * 0.002; // convert raw adc to voltage at divider
        float batteryVoltage = dividerVoltage * 2.0; // calculate actual battery voltage
        float clamped_voltage = constrain(batteryVoltage, 3.0, 4.2); // clamp voltage to 3.0v - 4.2v
        *_p_battery_level = map(clamped_voltage * 100, 300, 420, 0, 100); // map voltage to percentage
    }

    void readAmbientTempHumi() {
        if (!_p_temp_ambient || !_p_humidity) return; // check for valid pointers

        Wire.beginTransmission(AHT20_ADDR); // start transmission
        Wire.write(0xAC); // send measurement command
        Wire.write(0x33);
        Wire.write(0x00);
        Wire.endTransmission();
        delay(80); // wait for measurement

        Wire.requestFrom((uint8_t)AHT20_ADDR, (uint8_t)7); // request 7 bytes of data
        if (Wire.available() >= 7) { // if data is available
            byte data[7]; // create buffer
            for (int i=0; i < 7; i++) { data[i] = Wire.read(); } // read data into buffer

            unsigned long rawHumidity = ((unsigned long)data[1] << 16 | (unsigned long)data[2] << 8 | data[3]) >> 4; // parse raw humidity
            unsigned long rawTemp = (((unsigned long)data[3] & 0x0F) << 16 | (unsigned long)data[4] << 8 | data[5]); // parse raw temperature
            
            *_p_humidity = ( (float)rawHumidity / 1048576.0 ) * 100.0; // calculate final humidity
            *_p_temp_ambient = ( ( (float)rawTemp / 1048576.0 ) * 200.0 ) - 50.0; // calculate final temperature
        } else {
            Serial.println("Failed to read from AHT20 sensor!");
            *_p_temp_ambient = -99.9; // set error value
            *_p_humidity = -99.9;     // set error value
        }
    }

    void readLight() {
        if (!_p_light) return; // check for valid pointer
        Wire.beginTransmission(GY302_ADDR); // start transmission
        Wire.write(0x20); // send one-time high-res command
        Wire.endTransmission();
        delay(130); // wait for measurement
        Wire.requestFrom((uint8_t)GY302_ADDR, (uint8_t)2); // request 2 bytes
        if (Wire.available() >= 2) { // if data is available
            byte highByte = Wire.read(); // read high byte
            byte lowByte = Wire.read(); // read low byte
            unsigned int rawValue = (highByte << 8) | lowByte; // combine bytes
            *_p_light = rawValue / 1.2; // calculate lux
        } else {
            Serial.println("Failed to read from GY-302 sensor.");
            *_p_light = -1; // set error value
        }
    }

    void readSoilTemp() {
        if (!_p_temp_soil) return; // check for valid pointer
        int16_t adcValue = readADS1015(0); // read from adc channel 0
        float voltage = adcValue * 0.002; // convert adc to voltage
        float resistance = 10000.0 * ( (3.3 - voltage) / voltage ); // reversed voltage divider formula
        
        float steinhart; // variable for steinhart-hart calculation
        steinhart = resistance / 10000.0; // (r/r0)
        steinhart = log(steinhart);       // ln(r/r0)
        steinhart /= 3950.0;              // 1/b * ln(r/r0)
        steinhart += 1.0 / (25.0 + 273.15); // + (1/t0)
        steinhart = 1.0 / steinhart;      // invert
        *_p_temp_soil = steinhart - 273.15; // convert kelvin to celsius
    }

    void readSoilMoisture() {
        if (!_p_moisture) return; // check for valid pointer
        digitalWrite(SOIL_MOISTURE_EN_PIN, HIGH); // turn sensor on
        delay(500); // allow sensor to stabilize
        int16_t adcValue = readADS1015(1); // read from adc channel 1
        digitalWrite(SOIL_MOISTURE_EN_PIN, LOW); // turn sensor off
        *_p_moisture = map(adcValue, 0, 2047, 100, 0); // map raw value to percentage
    }

    void readSoilEC() {
        if (!_p_ec) return; // check for valid pointer
        // set pins to output for ac-like measurement
        pinMode(EC_PIN1, OUTPUT);
        pinMode(EC_PIN2, OUTPUT);
        delay(10);

        // first measurement
        digitalWrite(EC_PIN1, HIGH);
        digitalWrite(EC_PIN2, LOW);
        delay(15);
        int reading1 = readADS1015(3);

        // stabilize
        digitalWrite(EC_PIN1, LOW);
        digitalWrite(EC_PIN2, LOW);
        delay(15);
      
        // second measurement (reversed polarity)
        digitalWrite(EC_PIN1, LOW);
        digitalWrite(EC_PIN2, HIGH);
        delay(15);
        int reading2 = readADS1015(3);

        // turn probes off by setting pins to input
        digitalWrite(EC_PIN1, LOW);
        digitalWrite(EC_PIN2, LOW);
        pinMode(EC_PIN1, INPUT);
        pinMode(EC_PIN2, INPUT);

        // calculate average and resistance
        float avg_reading = (reading1 + (2047 - reading2)) / 2.0;
        float soil_resistance = (1000.0 * avg_reading) / (2047.0 - avg_reading);
        *_p_ec = 1000000.0 / soil_resistance; // convert resistance to conductivity (us/cm)
    }

    // === main Measurement Function ===
    void readAllSensors() {
        readBatteryLevel();
        readAmbientTempHumi();
        readLight();
        readSoilTemp();
        readSoilMoisture();
        readSoilEC();
    }
};