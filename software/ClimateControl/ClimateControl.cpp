#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <RTClib.h>

const int sdCardChipSelect = 10;
const int retryBeforeError = 2;

const int relayPin1 = 7;
const int relayPin2 = 8;

RTC_DS3231 rtc;

DHT dht_inside  (2, DHT22);
DHT dht_outside (3, DHT22);
int retryCounter = 0;
char filename[20];
char logline[255];

//File file;
//Sd2Card card;
//SdVolume volume;

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

bool isVentilationNeeded(float humidityInside, float humidityOutside) {
	return (humidityInside > humidityOutside);
}

void setVentilation(bool enabled) {
	Serial.println("setting ventilation");

	digitalWrite(relayPin1, !enabled);
	digitalWrite(relayPin2, !enabled);
}

void handleSensorError() {
	Serial.println("handling sensor error");
	// TODO add code
}

void setup() {
	Serial.begin(9600);
	delay(10000);
	Serial.print("Starting..");

	Wire.begin();

	// starting the sd card
	pinMode(sdCardChipSelect, OUTPUT);
	pinMode(relayPin1, OUTPUT);
	pinMode(relayPin2, OUTPUT);

	if (!SD.begin(sdCardChipSelect)) {
		Serial.println("SD-card initialization failed!");
	}

	// starting the realtime clock
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
float asAbsolute(float relativeHumidity, float temperature) {
	return (6.112 * pow(EULER, ((17.67 * temperature)/(243.5 + temperature))) * relativeHumidity * 2.1674) / (273.15 + temperature);
}

//void printCardInfo() {
//	// we'll use the initialization code from the utility libraries
//	  // since we're just testing if the card is working!
//	  if (!card.init(SPI_HALF_SPEED, sdCardChipSelect)) {
//	    Serial.println("initialization failed. Things to check:");
//	    Serial.println("* is a card inserted?");
//	    Serial.println("* is your wiring correct?");
//	    Serial.println("* did you change the chipSelect pin to match your shield or module?");
//	    return;
//	  } else {
//	    Serial.println("Wiring is correct and a card is present.");
//	  }
//
//	  // print the type of card
//	  Serial.print("\nCard type: ");
//	  switch (card.type()) {
//	    case SD_CARD_TYPE_SD1:
//	      Serial.println("SD1");
//	      break;
//	    case SD_CARD_TYPE_SD2:
//	      Serial.println("SD2");
//	      break;
//	    case SD_CARD_TYPE_SDHC:
//	      Serial.println("SDHC");
//	      break;
//	    default:
//	      Serial.println("Unknown");
//	  }
//
//	  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
//	  if (!volume.init(card)) {
//	    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
//	    return;
//	  }
//
//
//	  // print the type and size of the first FAT-type volume
//	  uint32_t volumesize;
//	  Serial.print("\nVolume type is FAT");
//	  Serial.println(volume.fatType(), DEC);
//	  Serial.println();
//
//	  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
//	  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
//	  volumesize *= 512;                            // SD card blocks are always 512 bytes
//	  Serial.print("Volume size (bytes): ");
//	  Serial.println(volumesize);
//	  Serial.print("Volume size (Kbytes): ");
//	  volumesize /= 1024;
//	  Serial.println(volumesize);
//	  Serial.print("Volume size (Mbytes): ");
//	  volumesize /= 1024;
//	  Serial.println(volumesize);
//}

void loop() {
	Serial.println("new iteration - reading rtc");
	// read sensor data
	DateTime now                = rtc.now();

	Serial.println("reading dht22 sensor");
	float    humidityInside     = dht_inside.readHumidity();
	float	 temperatureInside  = dht_inside.readTemperature();
	float    humidityOutside    = dht_outside.readHumidity();
	float	 temperatureOutside = dht_outside.readTemperature();

	Serial.print("Temperatures: ");
	Serial.print(temperatureInside);
	Serial.print(" - ");
	Serial.print(temperatureOutside);
	Serial.println("");

	Serial.print("valid read? ...");
	if (humidityInside != NAN && temperatureInside != NAN && humidityOutside != NAN && temperatureOutside != NAN) {
		retryCounter = 0;
		Serial.println(" yes");

		Serial.print("ventilation needed? ...");
		bool ventilationNeeded  = isVentilationNeeded(asAbsolute(humidityInside, temperatureInside), asAbsolute(humidityOutside, temperatureOutside));
		if (ventilationNeeded) {
			Serial.println(" yes");
		} else {
			Serial.println(" no");
		}

		// trigger ventilation and log to serial
		setVentilation(ventilationNeeded);

		// append logline to logfile for current date
		sprintf(filename, "%d%d%d.txt", now.year() % 10000, now.month() % 100, now.day() % 100);
		Serial.print("using logfile: ");
		Serial.println(filename);

		// timestamp;humid-out;temp-out;humid-in;temp-in;vent-set
//		sprintf(logline, "%d-%d-%dT%d:%d:%d;%f;%f;%f;%f", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second(),
//				humidityInside, temperatureInside, humidityOutside, temperatureOutside);
		Serial.print("calculated logline: ");
		Serial.println(logline);

		Serial.print("writing to logfile...");
		//printCardInfo();
		File file = SD.open("test.txt", FILE_WRITE);
		if (file) {
			//file.println(logline);
			file.println("hallo");
			file.flush();
			file.close();
			Serial.println(" done");
		} else {
			Serial.println(" failed");
		}
		Serial.println("");

	} else {
		if (retryCounter++ >= retryBeforeError) {
			retryCounter = 0;
			Serial.println(" no");
			handleSensorError();
		}
	}

	Serial.println("waiting for next iteration");
	// wait for sensors to get measure new values
	delay(5000);
}
