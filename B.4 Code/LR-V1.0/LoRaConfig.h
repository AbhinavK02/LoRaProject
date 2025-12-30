#ifndef _LORACONFIG_H_
#define _LORACONFIG_H_
// ############################### Library ######################################
#include <RadioLib.h>
#include <Preferences.h>

// ############################### Configuration and Definitions ######################################
// TTN Keys for Accessing the Network
#ifndef RADIOLIB_LORAWAN_JOIN_EUI // All zeros for TTN compliance
#define RADIOLIB_LORAWAN_JOIN_EUI 0x0000000000000000
#endif

#ifndef RADIOLIB_LORAWAN_DEV_EUI
#define RADIOLIB_LORAWAN_DEV_EUI 0x070B3D57ED0074D0
#endif

#ifndef RADIOLIB_LORAWAN_APP_KEY
#define RADIOLIB_LORAWAN_APP_KEY 0x02, 0xBC, 0xE7, 0xC1, 0x02, 0xB3, 0x18, 0xD7, 0x02, 0xF2, 0x18, 0xDF, 0x9E, 0x45, 0xD1, 0x8E
#endif

// LoRaWAN Parameters
#define LORAWAN_UPLINK_DATA_RATE 3

#define LORAWAN_UPLINK_DATA_MAX 115 // byte

// Payload Parameters
#define UPLINK_PAYLOAD_MAX_LEN 256
uint8_t uplinkPayload[UPLINK_PAYLOAD_MAX_LEN] = {0};
uint16_t uplinkPayloadLen = 0;

// Define IDs for payload encoding
#define ID_READY 0x07

// SX1262 pin order: Module(NSS/CS, DIO1, RESET, BUSY);
SX1262 radio = new Module(41, 39, 42, 40);

// For Storing nonces
Preferences store;

// Buffer for LoRaWAN nonces
uint8_t lwNonces[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];

// Buffer fpr Session
RTC_DATA_ATTR uint8_t lwSession[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];

// Selecting Europe Region
const LoRaWANBand_t Region = EU868;
const uint8_t subBand = 0;

// Copy over Keys
uint64_t joinEUI = RADIOLIB_LORAWAN_JOIN_EUI;
uint64_t devEUI = RADIOLIB_LORAWAN_DEV_EUI;
uint8_t appKey[] = {RADIOLIB_LORAWAN_APP_KEY};

// create the LoRaWAN node
LoRaWANNode node(&radio, &Region, subBand);

// ############################### Functions ######################################
// To handle error codes
String stateDecode(const int16_t result)
{
  switch (result)
  {
  case RADIOLIB_ERR_NONE:
    return "ERR_NONE";
  case RADIOLIB_ERR_CHIP_NOT_FOUND:
    return "ERR_CHIP_NOT_FOUND";
  case RADIOLIB_ERR_PACKET_TOO_LONG:
    return "ERR_PACKET_TOO_LONG";
  case RADIOLIB_ERR_RX_TIMEOUT:
    return "ERR_RX_TIMEOUT";
  case RADIOLIB_ERR_MIC_MISMATCH:
    return "ERR_MIC_MISMATCH";
  case RADIOLIB_ERR_INVALID_BANDWIDTH:
    return "ERR_INVALID_BANDWIDTH";
  case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:
    return "ERR_INVALID_SPREADING_FACTOR";
  case RADIOLIB_ERR_INVALID_CODING_RATE:
    return "ERR_INVALID_CODING_RATE";
  case RADIOLIB_ERR_INVALID_FREQUENCY:
    return "ERR_INVALID_FREQUENCY";
  case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
    return "ERR_INVALID_OUTPUT_POWER";
  case RADIOLIB_ERR_NETWORK_NOT_JOINED:
    return "RADIOLIB_ERR_NETWORK_NOT_JOINED";
  case RADIOLIB_ERR_DOWNLINK_MALFORMED:
    return "RADIOLIB_ERR_DOWNLINK_MALFORMED";
  case RADIOLIB_ERR_INVALID_REVISION:
    return "RADIOLIB_ERR_INVALID_REVISION";
  case RADIOLIB_ERR_INVALID_PORT:
    return "RADIOLIB_ERR_INVALID_PORT";
  case RADIOLIB_ERR_NO_RX_WINDOW:
    return "RADIOLIB_ERR_NO_RX_WINDOW";
  case RADIOLIB_ERR_INVALID_CID:
    return "RADIOLIB_ERR_INVALID_CID";
  case RADIOLIB_ERR_UPLINK_UNAVAILABLE:
    return "RADIOLIB_ERR_UPLINK_UNAVAILABLE";
  case RADIOLIB_ERR_COMMAND_QUEUE_FULL:
    return "RADIOLIB_ERR_COMMAND_QUEUE_FULL";
  case RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND:
    return "RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND";
  case RADIOLIB_ERR_JOIN_NONCE_INVALID:
    return "RADIOLIB_ERR_JOIN_NONCE_INVALID";
  case RADIOLIB_ERR_DWELL_TIME_EXCEEDED:
    return "RADIOLIB_ERR_DWELL_TIME_EXCEEDED";
  case RADIOLIB_ERR_CHECKSUM_MISMATCH:
    return "RADIOLIB_ERR_CHECKSUM_MISMATCH";
  case RADIOLIB_ERR_NO_JOIN_ACCEPT:
    return "RADIOLIB_ERR_NO_JOIN_ACCEPT";
  case RADIOLIB_LORAWAN_SESSION_RESTORED:
    return "RADIOLIB_LORAWAN_SESSION_RESTORED";
  case RADIOLIB_LORAWAN_NEW_SESSION:
    return "RADIOLIB_LORAWAN_NEW_SESSION";
  case RADIOLIB_ERR_NONCES_DISCARDED:
    return "RADIOLIB_ERR_NONCES_DISCARDED";
  case RADIOLIB_ERR_SESSION_DISCARDED:
    return "RADIOLIB_ERR_SESSION_DISCARDED";
  }
  return "See https://jgromes.github.io/RadioLib/group__status__codes.html";
}
// helper function to display any issues
void debug(bool failed, const __FlashStringHelper *message, int state, bool halt)
{
  if (failed)
  {
    Serial.print(message);
    Serial.print(" - ");
    Serial.print(stateDecode(state));
    Serial.print(" (");
    Serial.print(state);
    Serial.println(")");
    while (halt)
    {
      delay(1);
    }
  }
}
// helper function to display a byte array
void arrayDump(uint8_t *buffer, uint16_t len)
{
  for (uint16_t c = 0; c < len; c++)
  {
    char b = buffer[c];
    if (b < 0x10)
    {
      Serial.print('0');
    }
    Serial.print(b, HEX);
  }
  Serial.println();
}
// Activating LoRa and Additional Set up
int16_t lwActivate()
{
  int16_t state = RADIOLIB_ERR_UNKNOWN;

  // Setup the OTAA session information
  state = node.beginOTAA(joinEUI, devEUI, NULL, appKey);
  debug(state != RADIOLIB_ERR_NONE, F("Initialise node failed"), state, true);

  // Opening Memory
  store.begin("radiolib", false);

  // Restore Nonces from Flash
  if (store.isKey("nonces"))
  {
    Serial.println(F("Restoring LoRaWAN nonces"));
    store.getBytes("nonces", lwNonces, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
    state = node.setBufferNonces(lwNonces);
    debug(state != RADIOLIB_ERR_NONE, F("Restoring nonces failed"), state, false);

    // Restore session from RTC
    state = node.setBufferSession(lwSession);

    // Joining TTN (if restore not immediate)
    while (state != RADIOLIB_LORAWAN_NEW_SESSION)
    {
      Serial.println(F("Joining LoRaWAN..."));
      state = node.activateOTAA();
      if (state == RADIOLIB_LORAWAN_SESSION_RESTORED)
      {
        Serial.println(F("LoRaWAN session restored"));
        break;
      }
      else if (state == RADIOLIB_LORAWAN_NEW_SESSION)
      {
        Serial.println(F("Join successful, saving nonces"));
        uint8_t *persist = node.getBufferNonces();
        Serial.print("Dev Nonces: ");
        Serial.println((char *)persist);
        memcpy(lwNonces, persist, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
        store.putBytes("nonces", lwNonces, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
        break;
      }
      Serial.print(F("Join failed: "));
      Serial.println(state);
      uint8_t *persist = node.getBufferNonces();
      Serial.print("Dev Nonces: ");
      Serial.println((char *)persist);
      delay(5000); // Retry every 5s
    }
    store.end();
    return (state);
  }
  else
  {
    Serial.println(F("No stored nonces found"));
  }

  // If nonces not found, starting fresh
  state = RADIOLIB_ERR_NETWORK_NOT_JOINED;
  // Joining TTN
  while (state != RADIOLIB_LORAWAN_NEW_SESSION)
  {
    Serial.println(F("Joining LoRaWAN fresh..."));
    state = node.activateOTAA();

    // Storing Nonces
    uint8_t *persist = node.getBufferNonces();
    memcpy(lwNonces, persist, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
    store.putBytes("nonces", lwNonces, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);

    if (state == RADIOLIB_LORAWAN_NEW_SESSION)
      break;

    // Fail and Retry
    Serial.print(F("Join failed: "));
    Serial.println(state);
    delay(5000); // Retry every 5s
  }
  Serial.println(F("Join successful"));

  delay(1000);
  store.end();

  // Disable the ADR algorithm (on by default which is preferable)
  node.setADR(false);

  // Set a fixed datarate
  node.setDatarate(LORAWAN_UPLINK_DATA_RATE);

  // Manages uplink intervals to the TTN Fair Use Policy
  node.setDutyCycle(false);

  // Return the state for debugging
  return (state);
}
int16_t lwUplink(uint8_t *payloadBuffer)
{
  int16_t state = RADIOLIB_ERR_UNKNOWN;
  // Checking if payload is complete
  if (uplinkPayloadLen >= 2)
  { // if complete, upload
    // Printing upload info
    Serial.print("Uplink payload length: ");
    Serial.println(uplinkPayloadLen);
    // Output the uplink payload for debugging
    Serial.print("Uplink payload: ");
    for (int i = 0; i < uplinkPayloadLen; i++)
    {
      Serial.print(uplinkPayload[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // Uplink
    state = node.sendReceive(uplinkPayload, uplinkPayloadLen);
    while (state != RADIOLIB_ERR_DOWNLINK_MALFORMED && state != RADIOLIB_ERR_NONE)
    { // Loop until success
      Serial.println("Error in sendReceive:");
      Serial.println(state);
      delay(50);
      state = node.sendReceive(uplinkPayload, uplinkPayloadLen);
      delay(1000);
    }
    // Saving session to RTC memory
    if (state == RADIOLIB_ERR_NONE || state == RADIOLIB_ERR_DOWNLINK_MALFORMED)
    {
      Serial.print(F("FCntUp = "));
      Serial.println(node.getFCntUp());

      uint8_t *s = node.getBufferSession();
      memcpy(lwSession, s, RADIOLIB_LORAWAN_SESSION_BUF_SIZE);

      Serial.println("Sending uplink successful!");
      Serial.println();
    }
    // Return the state for debugging
    return (state);
  }
  else
  {
    Serial.println("Payload insufficient size");
    return (state);
  }
}
#endif