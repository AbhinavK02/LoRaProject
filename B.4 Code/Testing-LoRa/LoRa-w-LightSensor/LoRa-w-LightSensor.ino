// This code is just to test if I can stream the sensor data from the light sensor to the TTN via LoRa WAN

int sensor = 9;
int LED_BUILTIN = 18;

void setup() {
  Serial.begin(9600);
  pinMode(sensor,INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // int light = 0;
  // light = digitalRead(sensor);
  // Serial.println(light);
  // delay(50);

  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);                      // wait for a second
}