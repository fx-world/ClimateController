#include <Arduino.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>

const int sdCardChipSelect = 10;

#define SENSOR_READ_RETRIES 3
#define SENSOR_READ_RETRY_DELAY_MILLIS 10000

RTC_DS3231 rtc;

DHT dht_inside  (2, DHT22);
DHT dht_outside (3, DHT22);

struct SensorData {
	float humidity;
	float temperature;
};

SensorData insideData;
SensorData outsideData;


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

bool isVentilationNeeded(DateTime now, float humidityInside, float humidityOutside) {
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
	    Serial.println("RTC lost power, resetting!");
	    // following line sets the RTC to the date & time this sketch was compiled
	    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	}
}

/**
 * Convert relative humidity to absolute humidity.
 *
 * see: https://carnotcycle.wordpress.com/2012/08/04/how-to-convert-relative-humidity-to-absolute-humidity/
 */
float asAbsolute(SensorData data) {
	return (6.112 * pow(EULER, ((17.67 * data.temperature)/(243.5 + data.temperature))) * data.humidity * 2.1674) / (273.15 + data.temperature);
}

/**
 * read sensor data.
 * pause and retry if sensor data was NaN.
 */
void readSensors() {
	bool readError = false;
	int retry = SENSOR_READ_RETRIES;
	for (; retry > 0; retry--) {
		insideData.humidity = dht_inside.readHumidity();
		insideData.temperature = dht_inside.readTemperature();
		outsideData.humidity = dht_outside.readHumidity();
		outsideData.temperature = dht_inside.readTemperature();
		readError = isnan(insideData.humidity)
				 || isnan(insideData.temperature)
		 		 || isnan(outsideData.humidity)
		    	 || isnan(outsideData.temperature);
		if (readError) {
			delay(SENSOR_READ_RETRY_DELAY_MILLIS);
		} else {
			break;
		}
	}
}

void loop() {
	// read sensor data
	DateTime now                = rtc.now();

	readSensors();

	bool ventilationNeeded  = isVentilationNeeded(now, asAbsolute(insideData), asAbsolute(outsideData));

	// trigger ventilation and log to serial
	setVentilation(ventilationNeeded);

	// timestamp;humid-out;temp-out;humid-in;temp-in;vent-set
	char* logline = (char*) malloc(128 * sizeof(char));
	sprintf(logline, "%d-%d-%dT%d:%d:%d;%d;%d;%d;%d;%d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(),
			insideData.humidity, insideData.temperature, outsideData.humidity, outsideData.temperature, ventilationNeeded);

	// append logline to logfile for current date
	char* filename = (char*) malloc(14 * sizeof(char));
	sprintf(filename, "%d-%d-%d.txt", now.year(), now.month(), now.day());

	File file = SD.open(filename, O_APPEND);
	file.write(logline);
	file.close();

	Serial.println(logline);

	// wait for sensors to get measure new values
	delay(10000);
}
