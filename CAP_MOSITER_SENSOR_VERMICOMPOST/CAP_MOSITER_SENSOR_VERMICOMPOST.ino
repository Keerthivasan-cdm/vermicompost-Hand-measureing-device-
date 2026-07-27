/*
  DFRobot Capacitive Soil Moisture Sensor - ESP32-C3
  Reads raw ADC value, converts to moisture %, and detects if sensor is disconnected
*/

const int sensorPin = 0;   // GPIO0 (ADC1_CH0) — change to your wired pin (e.g. GPIO1, GPIO2, GPIO3, GPIO4)

// Calibration values — ESP32-C3 ADC range is 0-4095 (12-bit), NOT 0-1023
// These are rough defaults — CALIBRATE for your own sensor (see notes below)
const int airValue   = 2800;   // Raw ADC value in open air (fully dry)
const int waterValue = 1300;   // Raw ADC value fully submerged in water (fully wet)

// Disconnect detection thresholds
const int disconnectLowThreshold  = 50;    // Below this = floating/disconnected (pulled down)
const int disconnectHighThreshold = 4050;  // Near max = likely a wiring/short issue

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Enable internal pulldown so a disconnected sensor reads near 0 instead of floating randomly
  pinMode(sensorPin, INPUT_PULLDOWN);

  analogReadResolution(12);        // ESP32-C3 supports up to 12-bit (0-4095)
  analogSetAttenuation(ADC_11db);  // Allows full 0-3.3V range to be read

  Serial.println("DFRobot Capacitive Soil Moisture Sensor - ESP32-C3");
}

void loop() {
  int rawValue = analogRead(sensorPin);

  Serial.print("Raw ADC Value: ");
  Serial.print(rawValue);

  if (rawValue <= disconnectLowThreshold || rawValue >= disconnectHighThreshold) {
    // Sensor not detected — either unplugged or wiring fault
    Serial.println("   |   Status: SENSOR DISCONNECTED ⚠️");
  } else {
    int moisturePercent = map(rawValue, airValue, waterValue, 0, 100);
    moisturePercent = constrain(moisturePercent, 0, 100);

    Serial.print("   |   Soil Moisture: ");
    Serial.print(moisturePercent);
    Serial.println(" %");
  }

  delay(1000);
}