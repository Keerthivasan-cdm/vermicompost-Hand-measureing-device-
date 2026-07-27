#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------- Display (SPI) ----------
#define TFT_SCLK  8    // D8
#define TFT_MOSI  10   // D10
#define TFT_CS    5    // D2
#define TFT_DC    3    // D1
#define TFT_RST   20    // D3
#define TFT_BL    21   // D6

// ---------- I2C (RTC) ----------
#define I2C_SDA   6    // D4
#define I2C_SCL   7    // D5

// ---------- Sensors ----------
#define MOISTURE_PIN     4    // D0 - capacitive soil moisture (analog)
#define ONE_WIRE_BUS     9   // D7 - DS18B20 (digital, one-wire)

// Calibration for capacitive moisture sensor
#define MOISTURE_AIR_VALUE   2800   // raw ADC value in dry air  -> 0%
#define MOISTURE_WATER_VALUE 1300   // raw ADC value in water    -> 100%

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
RTC_DS3231 rtc;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Colors
#define COL_BG      ST77XX_BLACK
#define COL_FRAME   ST77XX_CYAN
#define COL_LABEL   ST77XX_WHITE
#define COL_VALUE   0x001F   // pure blue (RGB565)

unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 1000;

void drawStaticUI() {
  tft.fillScreen(COL_BG);

  // Outer decorative frame
  tft.drawRect(2, 2, 316, 236, COL_FRAME);
  tft.drawRect(4, 4, 312, 232, COL_FRAME);

  // Title bar
  tft.fillRect(6, 6, 308, 30, COL_FRAME);
  tft.setTextColor(COL_BG);
  tft.setTextSize(2);
  tft.setCursor(18, 14);
  tft.print("WELCOME Dhanush");

  // Divider line under title
  tft.drawFastHLine(6, 40, 308, COL_FRAME);

  // Labels
  tft.setTextSize(2);
  tft.setTextColor(COL_LABEL);
  tft.setCursor(20, 100);
  tft.print("Moisture:");

  tft.setCursor(20, 150);
  tft.print("Temp:");

  // Small decorative corner ticks
  tft.drawFastHLine(6, 230, 20, COL_FRAME);
  tft.drawFastHLine(294, 230, 20, COL_FRAME);
}

void updateDateTime() {
  DateTime now = rtc.now();
  char buf[16];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  char dbuf[16];
  snprintf(dbuf, sizeof(dbuf), "%02d/%02d/%04d", now.day(), now.month(), now.year());

  // Clear top-right area (inside title bar) before redrawing
  tft.fillRect(190, 8, 110, 26, COL_FRAME);
  tft.setTextColor(COL_BG);
  tft.setTextSize(1);
  tft.setCursor(196, 10);
  tft.print(dbuf);
  tft.setCursor(196, 22);
  tft.print(buf);
}

float readMoisturePercent() {
  int raw = analogRead(MOISTURE_PIN);
  raw = constrain(raw, MOISTURE_WATER_VALUE, MOISTURE_AIR_VALUE);
  int pct = map(raw, MOISTURE_AIR_VALUE, MOISTURE_WATER_VALUE, 0, 100);
  return pct;
}

float readTemperatureC() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(240, 320);
  tft.setRotation(1);

  Wire.begin(I2C_SDA, I2C_SCL);
  if (!rtc.begin()) {
    Serial.println("RTC not found!");
  }
  //rtc.adjust(DateTime(2026, 7, 23, 11, 45, 30)); // uncomment once to set time, then re-comment

  sensors.begin();

  drawStaticUI();
  Serial.println("Setup done.");
}

void loop() {
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();

    updateDateTime();

    float moisturePct = readMoisturePercent();
    float tempC = readTemperatureC();

    // Moisture value
    tft.fillRect(180, 95, 100, 24, COL_BG);
    tft.setTextSize(2);
    tft.setTextColor(COL_VALUE);
    tft.setCursor(180, 100);
    tft.print(moisturePct, 0);
    tft.print(" %");

    // Temperature value
    tft.fillRect(120, 145, 120, 24, COL_BG);
    tft.setTextColor(COL_VALUE);
    tft.setCursor(120, 150);
    tft.print(tempC, 1);
    tft.print(" C");

    Serial.print("Moisture: "); Serial.print(moisturePct);
    Serial.print(" %  Temp: "); Serial.print(tempC);
    Serial.println(" C");
  }
}