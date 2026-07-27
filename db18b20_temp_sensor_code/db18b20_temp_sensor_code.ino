#include <OneWire.h>
#include <DallasTemperature.h>

// GPIO pin connected to the DS18B20 data line
#define ONE_WIRE_BUS 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  pinMode(ONE_WIRE_BUS, INPUT_PULLUP);  // Enable internal pull-up
  sensors.begin();

  int deviceCount = sensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(deviceCount);
  Serial.println(" DS18B20 sensor(s).");

  if (deviceCount == 0) {
    Serial.println("No sensors found. Check wiring and pull-up resistor.");
  }
}

void loop() {
  sensors.requestTemperatures(); // Send command to get temperatures

  float tempC = sensors.getTempCByIndex(0);
  float tempF = sensors.getTempFByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: Could not read temperature data");
  } else {
    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.print(" °C  |  ");
    Serial.print(tempF);
    Serial.println(" °F");
  }

  delay(1000); // Update every 1 second
}