/* 
Author: Justin Julius Chin Cheong
Desc: This code is just to test if I can stream the sensor data from the light sensor to the TTN via LoRa WAN
      I also used a ton of code from RadioLib LoRaWAN starter example. 

*/

// #include "config.h"


// ------------------------ Variables -------------------------------------------------------------------------
// // SX1262 Pins
// #define RADIO_CS_PIN    41
// #define RADIO_DIO1_PIN  39
// #define RADIO_RST_PIN   42
// #define RADIO_BUSY_PIN  40

// SX1262 Object
// SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

// LoRaWAN Node Object
// LoRaWANNode node(&radio, &EU868);

// Sensors and Actuators 
int sensor = 8;
int led = 9;
// int LED_BUILTIN = 18;


void setup() {
  pinMode(sensor,INPUT);
  pinMode(led,OUTPUT);
  // pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  
  // Starting Radio 
  // Serial.println(F("Initialise the radio"));
  // int16_t state = radio.begin();
  // debug(state != RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, true);

  // Setup the OTAA session information
  // node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);

  // Serial.println(F("Join ('login') the LoRaWAN Network"));
  // state = node.activateOTAA();
  // debug(state != RADIOLIB_LORAWAN_NEW_SESSION, F("Join failed"), state, true);
  digitalWrite(led, HIGH);
  Serial.println(F("Ready!\n"));
}

void loop() {
  // Serial.println(F("Sending uplink"));

  // Read Sensor Data
  int light = digitalRead(sensor);
  Serial.println(light);
  // Build payload byte array
  // uint8_t uplinkPayload[1];
  // uplinkPayload[0] = light;
  // uplinkPayload[1] = highByte(value2);  
  // uplinkPayload[2] = lowByte(value2);

  // Perform an uplink
  // int16_t state = node.sendReceive(uplinkPayload, sizeof(uplinkPayload));    
  // debug((state != RADIOLIB_LORAWAN_NO_DOWNLINK) && (state != RADIOLIB_ERR_NONE), F("Error in sendReceive"), state, false);

  // Serial.print(F("Uplink complete, next in "));
  // Serial.print(uplinkIntervalSeconds);
  // Serial.println(F(" seconds"));
  
  // // Wait until next uplink - observing legal & TTN FUP constraints
  // delay(uplinkIntervalSeconds * 1000UL);  // delay needs milli-seconds
  delay(500);
}