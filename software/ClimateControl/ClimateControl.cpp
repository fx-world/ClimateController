#include <Arduino.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

const int sdCardChipSelect = 10;

RTC_DS3231 rtc;

DHT_Unified dht_inside  (2, DHT22);
DHT_Unified dht_outside (3, DHT22);

void showDate(const DateTime& date) {
	Serial.print(date.year(), DEC);
	Serial.print('/');
	Serial.print(date.month(), DEC);
	Serial.print('/');
	Serial.print(date.day(), DEC);
	Serial.print(' ');
	Serial.print(date.hour(), DEC);
	Serial.print(':');
	Serial.print(date.minute(), DEC);
	Serial.print(':');
	Serial.print(date.second(), DEC);
	Serial.println();
}

void setup() {
	Serial.begin(9600);
	Serial.print("Starting..");

	// starting the sd card
	pinMode(sdCardChipSelect, OUTPUT);

	if (!SD.begin(sdCardChipSelect)) {
		Serial.println("SD-card initialization failed!");
	}

	// starting the realtime clock
	Wire.begin();

	if (rtc.lostPower()) {
	    Serial.println("RTC lost power, reseting!");
	    // following line sets the RTC to the date & time this sketch was compiled
	    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	}
}

void loop() {
	DateTime now = rtc.now();
	showDate(now);

	sensors_event_t event;
	dht_inside.temperature().getEvent(&event);
	if (isnan(event.temperature)) {
		Serial.println("Error reading temperature!");
	} else {
		Serial.print("Temperature: ");
		Serial.print(event.temperature);
		Serial.println(" *C");
	}

	// Get humidity event and print its value.
	dht_inside.humidity().getEvent(&event);
	if (isnan(event.relative_humidity)) {
		Serial.println("Error reading humidity!");
	} else {
		Serial.print("Humidity: ");
		Serial.print(event.relative_humidity);
		Serial.println("%");
	}
}
