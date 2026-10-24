# Vermicompost Quality Monitor: Portable Handheld Device

A battery-powered handheld instrument that checks **vermicompost quality** in the field. It shows moisture and temperature on a 320 × 240 color TFT, with good / normal / bad status colors. The KiCad schematic for the device is included.

## Features
- **Soil moisture:** capacitive sensor, averaged over 10 samples, mapped to 0–100 %. Status bands: below 75 % is dry, 75–90 % is normal, 90 % or more is wet.
- **Temperature:** DS18B20 waterproof probe (OneWire).
- **Display:** ST7789 2.8" SPI TFT with a custom boot logo and live readings.
- **Field calibration tool:** a serial-command sketch with two-point dry/wet calibration. Values are saved to flash (`Preferences`), so they survive reboot.
- **Sensor fault detection:** flags a disconnected or shorted moisture probe and a missing DS18B20.
- **Battery powered:** Li-ion cell with TP4056 charging.
- **pH channel:** planned. In `vermi_partB_display` the pH value is currently **simulated** while the pH hardware is not yet wired.

## Hardware
| Part | Details |
|---|---|
| MCU | Seeed Studio XIAO ESP32-C3 |
| Moisture | Capacitive soil-moisture sensor (analog) |
| Temperature | DS18B20 + 4.7 kΩ pull-up |
| Display | ST7789 320 × 240 SPI TFT |
| Power | Li-ion cell, TP4056 charger module, power switch |
| Extra | Relay output, DS3231 RTC (test sketch) |

### Pin map (`vermi_calibration`, `vermi_partB_display`)
| Signal | GPIO |
|---|---|
| Moisture (ADC) | 2 |
| DS18B20 | 3 |
| TFT CS / DC / RST / BL | 6 / 5 / 9 / 4 |
| TFT MOSI / SCLK | 10 / 8 |
| Relay | 7 |

## Files
| File / folder | Description |
|---|---|
| `vermi_calibration.ino` | Calibration and live monitor (serial commands below) |
| `vermi_partB_display/` | Main device firmware with the TFT UI and logos |
| `VERMICOMPOST_WITHOUT_PH/` | Earlier version with RTC, without pH |
| `CAP_MOSITER_SENSOR_VERMICOMPOST/` | Moisture sensor test with disconnect detection |
| `db18b20_temp_sensor_code/` | DS18B20 test |
| `RTC_CODE_VERMICOMPOST/` | RTC test |
| `vermi_compost_fixed_20_8_26_1.7.pdf` / `.net` | KiCad 9 schematic (PDF) and netlist |

## Calibration (serial, 115200 baud)
| Key | Action |
|---|---|
| `d` | Record the DRY point (probe in dry air or soil) |
| `w` | Record the WET point (probe in water) |
| `t` | Set the temperature offset |
| `s` | Save the calibration to flash |
| `r` | Reset to defaults |
| `?` | Print the current calibration and raw values |

## Build and flash
1. Install the Arduino IDE with the **ESP32 board package** (select *XIAO_ESP32C3*).
2. Install the libraries: **Adafruit GFX**, **Adafruit ST7735 and ST7789**, **OneWire**, **DallasTemperature**.
3. Run `vermi_calibration.ino` once to calibrate, then flash `vermi_partB_display`.

## Author
**Keerthivasan J**, B.E. EEE, Sri Sairam Engineering College ([GitHub](https://github.com/Keerthivasan-cdm) · [LinkedIn](https://linkedin.com/in/keerthi-vasan-j-))
