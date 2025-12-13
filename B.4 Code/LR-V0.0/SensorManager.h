// SensorManager.h

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

// --- Configuration and Definitions ---
// Pins
#define LIGHT_SENSOR_PIN 2
#define LOADCELL_DOUT_PIN 43
#define LOADCELL_SCK_PIN 6
#define TILT_SENSOR_PIN 44

// Define IDs for payload encoding 
#define ID_WEIGHT_SENSOR 0x01
#define ID_LIGHT_SENSOR 0x02
#define ID_TILT_SENSOR 0x03   

// --- Data Structure ---
// A simple struct to hold one sensor's reading
struct SensorReading
{
    uint8_t id;    // Unique for each sensor ID 
    uint8_t value; // The actual value 
};

// --- Function Declarations ---

/**
 * @brief Initializes all sensor pins and hardware interfaces
 */
void sensorManager_init();

/**
 * @brief Reads the HX711 weight sensor
 * @return A SensorReading struct containing the sensor's data.
 */
SensorReading readWeightSensor();

/**
 * @brief Reads the Digital Light Sensor.
 * @return A SensorReading struct containing the sensor's data.
 */
SensorReading readLightSensor();

/**
 * @brief Reads the Digital Light Sensor.
 * @return A SensorReading struct containing the sensor's data.
 */
SensorReading readTiltSensor();

/**
 * @brief Collects data from all active sensors and builds the LoRaWAN payload.
 * @param payloadBuffer The buffer to write the payload bytes into.
 * @return The length of the resulting payload in bytes.
 */
uint16_t collectAndEncodeData(uint8_t *payloadBuffer);

#endif // SENSOR_MANAGER_H