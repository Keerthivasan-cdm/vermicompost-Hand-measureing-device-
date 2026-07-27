#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

void setup() {
  Serial.begin(9600);
  delay(1000);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC! Check wiring.");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time to compile time!");
    // Sets RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Uncomment the line below ONCE to manually set a specific time,
  // then re-comment it and re-upload, otherwise it resets every boot.
  // rtc.adjust(DateTime(2026, 7, 22, 14, 30, 0)); // YYYY, M, D, H, M, S
}

void loop() {
  DateTime now = rtc.now();

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek(now.dayOfTheWeek()));
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  // Bonus: read the onboard temperature sensor
  Serial.print("Temp: ");
  Serial.print(rtc.getTemperature());
  Serial.println(" C");

  Serial.println();
  delay(1000);
}

const char* daysOfTheWeek(int day) {
  const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                         "Thursday", "Friday", "Saturday"};
  return days[day];
}