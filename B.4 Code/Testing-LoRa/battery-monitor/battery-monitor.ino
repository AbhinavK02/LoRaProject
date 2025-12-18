#include <Arduino.h>
#define BAT_MONITOR_PIN 3
uint8_t value = 0;
void setup() {
  Serial.begin(115200);
  // Setting up ADC for Battery Monitor
  analogReadResolution(12);     // 0–4095
  analogSetAttenuation(ADC_11db); // Allows full 0–3.3V range

}

void loop() {
  int adcValue = analogRead(BAT_MONITOR_PIN);
  Serial.print("ADC Value: ");
  Serial.println(adcValue);
  float adcVoltage = (adcValue* 3.3) / 4095;
  Serial.print("ADC Voltage: ");
  Serial.println(adcVoltage);
  float batVoltage = adcVoltage * 1.5;
  Serial.print("Battery Voltage: ");
  Serial.println(batVoltage);
  if (batVoltage >= 4.20) {
      value = 64;
  } else if (batVoltage <= 3.10) {
      value = 0;
  } else {
      value = (batVoltage - 3.10) * 64 / (1.1);
  }
  Serial.print("Battery Percentage: ");
  Serial.println(value);
  Serial.println();
  delay(1000);
}
