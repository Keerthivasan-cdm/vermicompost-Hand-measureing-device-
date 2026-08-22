/*
  Vermi Compost - Part B
  Splash screens + live display + moisture-based relay control
  pH sensor not wired yet -> value is SIMULATED, biased around moisture level
  (kept inside 6.5 - 7.5, no real pH hardware read)

  Boot sequence:
    1. Agsaimo "LET'S GO GREEN!" logo   -> shown for 5 seconds
    2. Sri Sairam Techno Incubator logo + "VERMICOMPOST MONITORING" -> shown for 3 seconds
    3. Normal live sensor display

  Pinout:
    Moisture (analog)  -> GPIO2
    DS18B20 (OneWire)  -> GPIO3
    TFT CS             -> GPIO6
    TFT DC             -> GPIO5
    TFT RST            -> GPIO9
    TFT Backlight/EN   -> GPIO4
    Relay IN           -> GPIO7   (ACTIVE LOW - driving the pin LOW turns the relay ON)
    TFT MOSI           -> GPIO10
    TFT SCLK           -> GPIO8

  Moisture calibration values below are the ones you already captured
  with the calibration sketch (dry=2954, wet=1071). No cal button in
  this version - if you re-calibrate later, just update these two lines.

  Relay logic (hysteresis, since sensor readings jitter a little):
    moisture < 75%   -> pump ON  (turns relay on -> drives pin LOW)
    moisture >= 100%  -> pump OFF (drives pin HIGH)
    moisture 75-99%   -> leaves the pump in whatever state it was already in
  This keeps soil moisture bouncing inside the 75-100% band without the
  relay chattering on/off right at the edge.
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "logo.h"
#include "logo_agsaimo.h"

// ---------- Pins ----------
#define MOIST_PIN    2
#define ONE_WIRE_PIN 3
#define TFT_CS       6
#define TFT_DC       5
#define TFT_RST      9
#define TFT_EN       4   // backlight enable
#define RELAY_PIN    7
#define TFT_MOSI     10
#define TFT_SCLK     8

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

#define SCREEN_W 320
#define SCREEN_H 240

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature ds18b20(&oneWire);

// ---------- Moisture calibration (already captured, no cal flow needed) ----------
int dryValue = 2954;
int wetValue = 1071;

// ---------- Moving average for moisture ----------
const int SOIL_SAMPLES = 10;
int soilReadings[SOIL_SAMPLES];
int soilIndex = 0;
long soilSum = 0;
bool soilBufferFilled = false;

// ---------- Simulated pH ----------
float phValue = 7.0;

// ---------- Relay state (hysteresis) ----------
bool pumpOn = false;

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 2000;

// ---------- Colors ----------
#define COL_BORDER 0x07FF
#define COL_TITLE  0xFFE0
#define COL_LABEL  0xFFFF
#define COL_VALUE  0x87FF
#define COL_GOOD   0x07E0
#define COL_NORMAL 0xFD20
#define COL_BAD    0xF800

// ---------- Row geometry (same layout style as Part A) ----------
const int cardX = 8, cardY = 8, cardW = SCREEN_W - 16, cardH = SCREEN_H - 16;
const int titleH = 40;
const int rowY[3] = {100, 150, 200};

void centeredText(const char* text, int y, uint8_t size) {
  int16_t x1, y1; uint16_t w, h;
  tft.setTextSize(size);
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_W - w) / 2, y);
  tft.print(text);
}

int updateSoilAverage() {
  int raw = analogRead(MOIST_PIN);
  soilSum -= soilReadings[soilIndex];
  soilReadings[soilIndex] = raw;
  soilSum += raw;
  soilIndex = (soilIndex + 1) % SOIL_SAMPLES;
  if (soilIndex == 0) soilBufferFilled = true;
  int count = soilBufferFilled ? SOIL_SAMPLES : max(soilIndex, 1);
  return soilSum / count;
}

// Simulated pH: centered inside 6.5-7.5, nudged by moisture level,
// with a small random jitter so it doesn't look static on screen.
float simulatePh(int moisturePercent) {
  float base = 6.5 + (moisturePercent / 100.0) * 1.0; // maps 0-100% -> 6.5-7.5
  float jitter = ((random(-10, 11)) / 100.0);          // +/- 0.10
  float val = base + jitter;
  if (val < 6.5) val = 6.5;
  if (val > 7.5) val = 7.5;
  return val;
}

void statusText(float value, float goodMin, float goodMax, float okMin, float okMax,
                 char* buf, uint16_t &color) {
  if (value >= goodMin && value <= goodMax) {
    strcpy(buf, "(Good)");
    color = COL_GOOD;
  } else if (value >= okMin && value <= okMax) {
    strcpy(buf, "(Normal)");
    color = COL_NORMAL;
  } else {
    strcpy(buf, "(Low)");
    color = COL_BAD;
  }
}

// pump ON -> relay active -> pin driven LOW (signal is inverted)
// pump OFF -> pin driven HIGH
void updateRelay(int moisturePercent) {
  if (moisturePercent < 75) {
    pumpOn = true;
  } else if (moisturePercent >= 90) {
    pumpOn = false;
  }
  // 75-99% -> leave pumpOn as-is (hysteresis band)
  digitalWrite(RELAY_PIN, pumpOn ? LOW : HIGH);
}

void drawStaticLayout() {
  tft.fillScreen(ST77XX_BLACK);

  tft.drawRoundRect(cardX, cardY, cardW, cardH, 10, COL_BORDER);
  tft.drawRoundRect(cardX + 1, cardY + 1, cardW - 2, cardH - 2, 9, COL_BORDER);

  tft.setTextColor(COL_TITLE);
  tft.setTextSize(2);
  tft.setCursor(cardX + 15, cardY + 12);
  tft.print("Vermi Compost");

  tft.drawFastHLine(cardX + 2, cardY + titleH, cardW - 4, COL_BORDER);

  tft.setTextSize(2);
  tft.setTextColor(COL_LABEL);

  tft.setCursor(cardX + 15, rowY[0] - 10);
  tft.print("Moisture");

  tft.setCursor(cardX + 15, rowY[1] - 10);
  tft.print("Temp");

  tft.setCursor(cardX + 15, rowY[2] - 10);
  tft.print("PH");

  tft.drawFastHLine(cardX + 2, rowY[0] + 20, cardW - 4, 0x39C7);
  tft.drawFastHLine(cardX + 2, rowY[1] + 20, cardW - 4, 0x39C7);
}

// ---------- Boot splash 1: Agsaimo logo, shown for 5s ----------
void showAgsaimoSplash() {
  tft.fillScreen(ST77XX_BLACK);

  int logoX = (SCREEN_W - LOGO2_WIDTH) / 2;
  int logoY = (SCREEN_H - LOGO2_HEIGHT) / 2;
  tft.drawRGBBitmap(logoX, logoY, logo2_bitmap, LOGO2_WIDTH, LOGO2_HEIGHT);

  delay(5000);
}

// ---------- Boot splash 2: incubator logo + "VERMICOMPOST MONITORING" for 3s ----------
void showSplashScreen() {
  tft.fillScreen(ST77XX_BLACK);

  int logoX = (SCREEN_W - LOGO_WIDTH) / 2;
  int logoY = 15;
  tft.drawRGBBitmap(logoX, logoY, logo_bitmap, LOGO_WIDTH, LOGO_HEIGHT);

  tft.setTextColor(COL_TITLE);
  centeredText("VERMICOMPOST", logoY + LOGO_HEIGHT + 18, 2);
  centeredText("MONITORING", logoY + LOGO_HEIGHT + 40, 2);

  delay(3000); // at least 3 seconds on screen
}

void updateNormalDisplay() {
  int soilRawAvg = updateSoilAverage();
  int moisturePercent = map(soilRawAvg, dryValue, wetValue, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);

  ds18b20.requestTemperatures();
  float tempC = ds18b20.getTempCByIndex(0);

  phValue = simulatePh(moisturePercent);

  updateRelay(moisturePercent);

  char statBuf[10];
  uint16_t statColor;

  // ---- Moisture row ----
  tft.fillRect(cardX + 130, rowY[0] - 16, cardW - 140, 24, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(COL_VALUE);
  tft.setCursor(cardX + 130, rowY[0] - 14);
  tft.printf("%d", moisturePercent);
  statusText(moisturePercent, 75, 100, 50, 74, statBuf, statColor);
  tft.setTextColor(statColor);
  tft.setCursor(cardX + 190, rowY[0] - 14);
  tft.print(statBuf);

  // ---- Temp row ----
  tft.fillRect(cardX + 130, rowY[1] - 16, cardW - 140, 24, ST77XX_BLACK);
  tft.setTextColor(COL_VALUE);
  tft.setCursor(cardX + 130, rowY[1] - 14);
  if (tempC == DEVICE_DISCONNECTED_C) {
    tft.setTextColor(COL_BAD);
    tft.print("Err");
  } else {
    tft.printf("%.1f", tempC);
    statusText(tempC, 20, 30, 15, 35, statBuf, statColor);
    tft.setTextColor(statColor);
    tft.setCursor(cardX + 190, rowY[1] - 14);
    tft.print(statBuf);
  }

  // ---- PH row (simulated) ----
  tft.fillRect(cardX + 130, rowY[2] - 16, cardW - 140, 24, ST77XX_BLACK);
  tft.setTextColor(COL_VALUE);
  tft.setCursor(cardX + 130, rowY[2] - 14);
  tft.printf("%.1f", phValue);
  statusText(phValue, 6.5, 7.5, 6.5, 7.5, statBuf, statColor);
  tft.setTextColor(statColor);
  tft.setCursor(cardX + 190, rowY[2] - 14);
  tft.print(statBuf);

  // ---- Pump status line ----
  tft.fillRect(cardX + 15, cardH - 6, cardW - 30, 16, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(pumpOn ? COL_GOOD : COL_NORMAL);
  tft.setCursor(cardX + 15, cardH - 2);
  tft.print(pumpOn ? "PUMP: ON (watering)" : "PUMP: OFF");
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // start with pump OFF (inverted signal)

  pinMode(TFT_EN, OUTPUT);
  digitalWrite(TFT_EN, HIGH); // backlight on

  tft.init(240, 320);
  tft.setRotation(1);

  randomSeed(analogRead(MOIST_PIN)); // seed RNG for the simulated pH jitter

  showAgsaimoSplash();
  showSplashScreen();

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  ds18b20.begin();

  for (int i = 0; i < SOIL_SAMPLES; i++) {
    int r = analogRead(MOIST_PIN);
    soilReadings[i] = r;
    soilSum += r;
    delay(10);
  }
  soilBufferFilled = true;

  drawStaticLayout();
}

void loop() {
  unsigned long now = millis();
  if (now - lastUpdate >= updateInterval) {
    lastUpdate = now;
    updateNormalDisplay();
  }
}
