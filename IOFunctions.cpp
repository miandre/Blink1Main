// 
// 
// 

#include "HeaderFiles/IOFunctions.h"

void captureBlink(uint8_t ledPin) {
	digitalWrite(ledPin, HIGH);
	delay(100);
	digitalWrite(ledPin, LOW);
	delay(125);
	digitalWrite(ledPin, HIGH);
	delay(100);
	digitalWrite(ledPin, LOW);
	delay(675);
}
void confirmBlink(uint8_t ledPin) {
	for (int i = 0; i < 5; i++) {
		digitalWrite(ledPin, HIGH);
		delay(100);
		digitalWrite(ledPin, LOW);
		delay(100);
	}
}

void capturePulse(uint8_t ledPin) {
	uint8_t brightness = 255;
	uint8_t fadeAmount = 5;
	while(brightness > 1) {
		analogWrite(ledPin, brightness);

		brightness = brightness - fadeAmount;

		// wait for 30 milliseconds to see the dimming effect
		delay(30);
	}
	while (brightness <255) {
		analogWrite(ledPin, brightness);

		brightness = brightness + fadeAmount;

		// wait for 30 milliseconds to see the dimming effect
		delay(30);
	}
	delay(100);

}

void nBlink(uint8_t ledPin, uint8_t n) {
	for (int i = 0; i < n; i++) {
		digitalWrite(ledPin, HIGH);
		delay(150);
		digitalWrite(ledPin, LOW);
		delay(150);
	}
}


void flushSerial() {
	while (Serial.available()) {
		Serial.read();
	}

}