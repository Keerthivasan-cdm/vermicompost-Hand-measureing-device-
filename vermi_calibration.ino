/*
  Vermi Compost - Part B
  SENSOR CALIBRATION + LIVE MONITOR SKETCH
  (pH sensor deferred for now - not wired in this version)

  Pinout (as given):
    Moisture (analog)  -> GPIO2
    DS18B20 (OneWire)  -> GPIO3
    TFT CS             -> GPIO6
    TFT DC             -> GPIO5
    TFT RST            -> GPIO9
    TFT Backlight/EN   -> GPIO4
    Relay IN           -> GPIO7
    TFT MOSI           -> GPIO10
    TFT SCLK           -> GPIO8

  ---------------------------------------------------------
  HOW TO CALIBRATE (open Serial Monitor @ 115200 baud):
  ---------------------------------------------------------
    d   -> hold moisture probe in DRY air/soil, send 'd' to capture DRY point
    w   -> dip moisture probe in WATER/saturated soil, send 'w' to capture WET point
    t   -> set a temperature offset trim (it will ask you to type a number + Enter)
    s   -> SAVE current calibration to flash (survives reboot/power loss)
    r   -> reset calibration to defaults (still need to send 's' after to persist)
    1   -> turn relay ON  (wiring test)
    0   -> turn relay OFF (wiring test)
    ?   -> print current calibration + live raw values

  The TFT screen shows live moisture %, raw ADC, temperature, and the
  current dry/wet calibration points at all times, so you can watch the
  raw value change in real time while you calibrate.
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>

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

Preferences prefs;

// ---------- Calibration values (defaults, overwritten from flash if saved) ----------
int dryValue   = 2400;   // raw ADC reading in dry air / dry soil
int wetValue   = 1200;   // raw ADC reading in water / saturated soil
float tempOffset = 0.0;  // additive trim applied to DS18B20 reading

// ---------- Moving average for moisture ----------
const int SOIL_SAMPLES = 10;
int soilReadings[SOIL_SAMPLES];
int soilIndex = 0;
long soilSum = 0;
bool soilBufferFilled = false;

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 1000;

// ---------- Colors ----------
#define COL_BORDER 0x07FF
#define COL_TITLE  0xFFE0
#define COL_LABEL  0xFFFF
#define COL_VALUE  0x87FF
#define COL_GOOD   0x07E0
#define COL_WARN   0xFD20
#define COL_BAD    0xF800

void centeredText(const char* text, int y, uint8_t size) {
  int16_t x1, y1; uint16_t w, h;
  tft.setTextSize(size);
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_W - w) / 2, y);
  tft.print(text);
}

int readSoilRaw() {
  int raw = analogRead(MOIST_PIN);
  soilSum -= soilReadings[soilIndex];
  soilReadings[soilIndex] = raw;
  soilSum += raw;
  soilIndex = (soilIndex + 1) % SOIL_SAMPLES;
  if (soilIndex == 0) soilBufferFilled = true;
  int count = soilBufferFilled ? SOIL_SAMPLES : max(soilIndex, 1);
  return soilSum / count;
}

void loadCalibration() {
  prefs.begin("vermi", true); // read-only
  dryValue   = prefs.getInt("dry", dryValue);
  wetValue   = prefs.getInt("wet", wetValue);
  tempOffset = prefs.getFloat("toff", tempOffset);
  prefs.end();
}

void saveCalibration() {
  prefs.begin("vermi", false); // read-write
  prefs.putInt("dry", dryValue);
  prefs.putInt("wet", wetValue);
  prefs.putFloat("toff", tempOffset);
  prefs.end();
  Serial.println("Calibration saved to flash.");
}

void resetCalibration() {
  dryValue = 2400;
  wetValue = 1200;
  tempOffset = 0.0;
  Serial.println("Calibration reset to defaults (send 's' to persist).");
}

void printStatus(int raw, float tempC) {
  Serial.println("---- Calibration status ----");
  Serial.printf("Dry point : %d\n", dryValue);
  Serial.printf("Wet point : %d\n", wetValue);
  Serial.printf("Temp offset: %.2f C\n", tempOffset);
  Serial.printf("Live raw moisture: %d\n", raw);
  Serial.printf("Live temp (with offset): %.2f C\n", tempC);
  Serial.println("Commands: d=set dry  w=set wet  t=set temp offset  s=save  r=reset  1/0=relay  ?=status");
}

void handleSerial(int currentRaw) {
  if (!Serial.available()) return;
  char c = Serial.read();
  switch (c) {
    case 'd': case 'D':
      dryValue = currentRaw;
      Serial.printf("Dry point captured: %d (probe should be in DRY air/soil)\n", dryValue);
      break;
    case 'w': case 'W':
      wetValue = currentRaw;
      Serial.printf("Wet point captured: %d (probe should be in WATER/saturated soil)\n", wetValue);
      break;
    case 't': case 'T': {
      Serial.println("Enter temp offset in C (e.g. -0.5) then press Enter:");
      unsigned long start = millis();
      while (!Serial.available() && millis() - start < 10000) delay(10);
      if (Serial.available()) {
        float v = Serial.parseFloat();
        tempOffset = v;
        Serial.printf("Temp offset set to %.2f C\n", tempOffset);
      } else {
        Serial.println("Timed out, offset unchanged.");
      }
      break;
    }
    case 's': case 'S':
      saveCalibration();
      break;
    case 'r': case 'R':
      resetCalibration();
      break;
    case '1':
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("Relay ON");
      break;
    case '0':
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Relay OFF");
      break;
    case '?':
      printStatus(currentRaw, ds18b20.getTempCByIndex(0) + tempOffset);
      break;
    default:
      break; // ignore newlines / unknown chars
  }
}

void drawStaticLayout() {
  tft.fillScreen(ST77XX_BLACK);
  tft.drawRoundRect(8, 8, SCREEN_W - 16, SCREEN_H - 16, 10, COL_BORDER);
  centeredText("VERMI COMPOST", 18, 2);
  centeredText("Sensor Calibration Mode", 42, 1);
  tft.drawFastHLine(10, 60, SCREEN_W - 20, COL_BORDER);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // relay OFF at boot

  pinMode(TFT_EN, OUTPUT);
  digitalWrite(TFT_EN, HIGH); // backlight on

  tft.init(240, 320);
  tft.setRotation(1);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  ds18b20.begin();

  loadCalibration();

  for (int i = 0; i < SOIL_SAMPLES; i++) {
    int r = analogRead(MOIST_PIN);
    soilReadings[i] = r;
    soilSum += r;
    delay(10);
  }
  soilBufferFilled = true;

  drawStaticLayout();

  Serial.println("Vermi Compost calibration sketch ready.");
  printStatus(readSoilRaw(), 0);
}

void loop() {
  int raw = readSoilRaw();
  handleSerial(raw);

  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();

    ds18b20.requestTemperatures();
    float tempC = ds18b20.getTempCByIndex(0) + tempOffset;

    int moisturePercent = map(raw, dryValue, wetValue, 0, 100);
    moisturePercent = constrain(moisturePercent, 0, 100);

    tft.fillRect(15, 70, SCREEN_W - 30, 140, ST77XX_BLACK);

    tft.setTextColor(COL_LABEL);
    tft.setTextSize(2);
    tft.setCursor(20, 75);
    tft.print("Moisture:");
    tft.setTextColor(COL_VALUE);
    tft.setCursor(180, 75);
    tft.printf("%d %%", moisturePercent);

    tft.setTextColor(COL_LABEL);
    tft.setCursor(20, 100);
    tft.print("Raw ADC:");
    tft.setTextColor(COL_VALUE);
    tft.setCursor(180, 100);
    tft.printf("%d", raw);

    tft.setTextColor(COL_LABEL);
    tft.setCursor(20, 125);
    tft.print("Temp:");
    tft.setTextColor(COL_VALUE);
    tft.setCursor(180, 125);
    if (tempC < -100) {
      tft.setTextColor(COL_BAD);
      tft.print("Sensor Err");
    } else {
      tft.printf("%.1f C", tempC);
    }

    tft.setTextSize(1);
    tft.setTextColor(COL_LABEL);
    tft.setCursor(20, 150);
    tft.print("Dry/Wet pt:");
    tft.setTextColor(COL_VALUE);
    tft.setCursor(140, 150);
    tft.printf("%d / %d", dryValue, wetValue);

    tft.setTextColor(digitalRead(RELAY_PIN) ? COL_GOOD : COL_WARN);
    tft.setCursor(20, 175);
    tft.print(digitalRead(RELAY_PIN) ? "RELAY: ON" : "RELAY: OFF");

    tft.setTextColor(COL_LABEL);
    tft.setCursor(20, 200);
    tft.print("Send d/w/t/s/r/1/0/? over Serial");
  }
}
