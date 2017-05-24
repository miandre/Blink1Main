// IOFunctions.h

#ifndef _IOFUNCTIONS_h
#define _IOFUNCTIONS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
void captureBlink(uint8_t ledPin);
void confirmBlink(uint8_t ledPin);
void capturePulse(uint8_t ledPin);
void nBlink(uint8_t ledPin, uint8_t n);
void flushSerial();

#endif

