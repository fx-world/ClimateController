#include <Arduino.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>

const int sdCardChipSelect = 10;

RTC_DS3231 rtc;

DHT dht_inside  (2, DHT22);
DHT dht_outside (3, DHT22);

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

bool isVentialationNeeded(float humidityInside, float humidityOutside) {
	return (humidityInside > humidityOutside);
}

void setVentilation(bool enabled) {
	// TODO add code
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
	float    humidityInside  = dht_inside.readHumidity();
	float    humidityOutside = dht_outside.readHumidity();
	DateTime now             = rtc.now();

	showDate(now);

	// TODO add delay for activation and deactivation, so it doesn't always turns off and on
	if (isVentialationNeeded(humidityInside, humidityOutside)) {
		setVentilation(true);
	} else {
		setVentilation(false);
	}

	delay(10000);
}
