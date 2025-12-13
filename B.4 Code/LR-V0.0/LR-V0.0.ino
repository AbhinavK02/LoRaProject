#include "config.h"
#include "EEPROM.h"
#include <Wire.h>
#include "SensorManager.h"

// Selecting Europe Region
const LoRaWANBand_t Region = EU868;
const uint8_t subBand = 0; 

// SX1262 pin order: Module(NSS/CS, DIO1, RESET, BUSY);
SX1262 radio = new Module(41, 39, 42, 40);

// create the LoRaWAN node
LoRaWANNode node(&radio, &Region, subBand);

uint64_t joinEUI =   RADIOLIB_LORAWAN_JOIN_EUI;
uint64_t devEUI  =   RADIOLIB_LORAWAN_DEV_EUI;
uint8_t appKey[] = { RADIOLIB_LORAWAN_APP_KEY };
uint8_t nwkKey[] = { RADIOLIB_LORAWAN_NWK_KEY };

#define LORAWAN_DEV_INFO_SIZE 36
uint8_t deviceInfo[LORAWAN_DEV_INFO_SIZE] = {0};

#define SERIAL_DATA_BUF_LEN  64
uint8_t serialDataBuf[SERIAL_DATA_BUF_LEN] = {0};
uint8_t serialIndex = 0;

#define UPLINK_PAYLOAD_MAX_LEN  256
uint8_t uplinkPayload[UPLINK_PAYLOAD_MAX_LEN] = {0};
uint16_t uplinkPayloadLen = 0;

uint32_t previousMillis = 0;

void setup() {
  Serial.begin(115200);

  sensorManager_init();

  // For Storing Device Info
  if(!EEPROM.begin(LORAWAN_DEV_INFO_SIZE))
  {
    Serial.println("Failed to initialize EEPROM");
    while(1);
  }

  uint32_t now = millis();
  while(1)
  {
    deviceInfoSet();
    if(millis() - now >= 5000) break;
  }

  // Radio Setup
  deviceInfoLoad();
  Serial.println(F("\nSetup... "));  
  Serial.println(F("Initialise the radio"));
  int16_t state = radio.begin();
  debug(state!= RADIOLIB_ERR_NONE, F("Initialise radio failed"), state, true);

  // SX1262 rf switch order: setRfSwitchPins(rxEn, txEn);
  radio.setRfSwitchPins(38, RADIOLIB_NC);

  // Setup the OTAA session information
  node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);
  Serial.println(F("Join ('login') the LoRaWAN Network"));

  // Joining TTN 
  while(1)
  {
    state = node.activateOTAA(LORAWAN_UPLINK_DATA_RATE);
    if(state == RADIOLIB_LORAWAN_NEW_SESSION) break;
    debug(state!= RADIOLIB_LORAWAN_NEW_SESSION, F("Join failed"), state, true);
    delay(15000);
  }

  // Disable the ADR algorithm (on by default which is preferable)
  node.setADR(false);

  // Set a fixed datarate
  node.setDatarate(LORAWAN_UPLINK_DATA_RATE);

  // Manages uplink intervals to the TTN Fair Use Policy
  node.setDutyCycle(false);
  
  // Ready to being streaming
  Serial.println(F("Ready!\n"));

  Wire.begin();
}

void loop() {
  // Reading Sensors and Storing in Payload
  uplinkPayloadLen = collectAndEncodeData(uplinkPayload);

  // Printing upload info
  Serial.print("Uplink payload length: ");
  Serial.println(uplinkPayloadLen);
  // Output the uplink payload for debugging
  Serial.print("Uplink payload: ");
  for (int i = 0; i < uplinkPayloadLen; i++) {
    Serial.print(uplinkPayload[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Checking LoRa State and Uploading
  int16_t state = node.sendReceive(uplinkPayload, uplinkPayloadLen, LORAWAN_UPLINK_USER_PORT);
  if (state!= RADIOLIB_LORAWAN_NO_DOWNLINK && state!= RADIOLIB_ERR_NONE) {
    Serial.println("Error in sendReceive:");
    Serial.println(state);
  } else {
    Serial.println("Sending uplink successful!");
  }

  // Checking if it is time to send again
  uint32_t currentMillis = millis();
  if(currentMillis - previousMillis >= LORAWAN_UPLINK_PERIOD)
  {
    previousMillis = currentMillis;
    if(uplinkPayloadLen)
    {
      Serial.println(F("Sending uplink"));
      int16_t state = node.sendReceive(uplinkPayload, uplinkPayloadLen, LORAWAN_UPLINK_USER_PORT);
      debug((state!= RADIOLIB_LORAWAN_NO_DOWNLINK) && (state!= RADIOLIB_ERR_NONE), F("Error in sendReceive"), state, false);
      uplinkPayloadLen = 0;
    }
  }
  delay(1000);
}

void deviceInfoLoad() {
  uint16_t checkSum = 0, checkSum_ = 0;
  for(int i = 0; i < LORAWAN_DEV_INFO_SIZE; i++) deviceInfo[i] = EEPROM.read(i);
  for(int i = 0; i < 32; i++) checkSum += deviceInfo[i];
  memcpy((uint8_t *)(&checkSum_), deviceInfo + 32, 2);

  if(checkSum == checkSum_)
  {
    memcpyr((uint8_t *)(&joinEUI), deviceInfo, 8);
    memcpyr((uint8_t *)(&devEUI), deviceInfo + 8, 8);
    memcpy(appKey, deviceInfo + 16, 16);

    Serial.println("Load device info:");
    Serial.print("JoinEUI:");
    Serial.println(joinEUI, HEX);
    Serial.print("DevEUI:");
    Serial.println(devEUI, HEX);
    Serial.print("AppKey:");
    arrayDump(appKey, 16);
    Serial.print("nwkKey:");
    arrayDump(nwkKey, 16);
  }
  else
  {
    Serial.println("Use the default device info as LoRaWAN param");
  }
}

void deviceInfoSet() {
  if(Serial.available())
  {
    serialDataBuf[serialIndex++] = Serial.read();
    if(serialIndex >= SERIAL_DATA_BUF_LEN) serialIndex = 0;
    if(serialIndex > 2 && serialDataBuf[serialIndex - 2] == '\r' && serialDataBuf[serialIndex-1] == '\n')
    {
      Serial.println("Get serial data:");
      arrayDump(serialDataBuf, serialIndex);
      if(serialIndex == 34) // 8 + 8 + 16 + 2
      {
        uint16_t checkSum = 0;
        for(int i = 0; i < 32; i++) checkSum += serialDataBuf[i];
        memcpy(deviceInfo, serialDataBuf, 32);
        memcpy(deviceInfo + 32, (uint8_t *)(&checkSum), 2);
        for(int i = 0; i < 34; i++) EEPROM.write(i, deviceInfo[i]);
        EEPROM.commit();
        Serial.println("Save serial data, please reboot...");
      }
      else
      {
        Serial.println("Error serial data length");
      }

      serialIndex = 0;
      memset(serialDataBuf, sizeof(serialDataBuf), 0);
    }
  }
}