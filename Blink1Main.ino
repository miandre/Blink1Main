/*

*/

#include <Adafruit_FONA.h>
#define RED_BUTTON_PIN 2
#define GREEN_BUTTON_PIN 3
#define RED_LED_PIN 8
#define GREEN_LED_PIN 12

enum State {
	NEUTRAL,
	GREEN_CAPTURING,
	GREEN_AWAITING_CONNFIRM,
	GREEN_HAS_BASE,
	RED_CAPTURING,
	RED_AWAITING_CONNFIRM,
	RED_HAS_BASE
};

boolean greenButtonToggle = true;
boolean redButtonToggle = true;
volatile State state;

void setup() {
	pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
	pinMode(GREEN_BUTTON_PIN, INPUT_PULLUP);
	pinMode(GREEN_LED_PIN, OUTPUT);
	pinMode(RED_LED_PIN, OUTPUT);
	attachInterrupt(digitalPinToInterrupt(GREEN_BUTTON_PIN), greenButtonISR, CHANGE);
	attachInterrupt(digitalPinToInterrupt(RED_BUTTON_PIN), redButtonISR, CHANGE);
	pinMode(LED_BUILTIN, OUTPUT);

	Serial.begin(115200);
	while (!Serial) {
		; // wait for serial port to connect. Needed for native USB port only
	}
	Serial.println("\n\nConnected!\n");
	Serial.println();

	state = NEUTRAL;
}

void loop() {

	while (state == NEUTRAL) {
		digitalWrite(RED_LED_PIN, LOW);
		digitalWrite(GREEN_LED_PIN, LOW);
	}

	while (state == GREEN_CAPTURING) {
		digitalWrite(RED_LED_PIN, LOW);
		int16_t capturingCountdown = 5;
		while (capturingCountdown > 0 && state == GREEN_CAPTURING) {
			digitalWrite(GREEN_LED_PIN, HIGH);
			delay(200);
			digitalWrite(GREEN_LED_PIN, LOW);
			delay(400);
			capturingCountdown--;
		}
		state = GREEN_AWAITING_CONNFIRM;
	}

	while (state == GREEN_AWAITING_CONNFIRM) {
		digitalWrite(RED_LED_PIN, LOW);
		int16_t confirmTimeout = 20;

		while (confirmTimeout > 0 && state == GREEN_AWAITING_CONNFIRM) {
			digitalWrite(GREEN_LED_PIN, HIGH);
			delay(100);
			digitalWrite(GREEN_LED_PIN, LOW);
			delay(100);
			confirmTimeout--;
		}
		if (state == GREEN_AWAITING_CONNFIRM) {
			state = NEUTRAL;
		}
	}

	while (state == GREEN_HAS_BASE) {
		digitalWrite(RED_LED_PIN, LOW);
		digitalWrite(GREEN_LED_PIN, HIGH);
	}

	while (state == RED_CAPTURING) {
		digitalWrite(GREEN_LED_PIN, LOW);
		int16_t capturingCountdown = 5;
		while (capturingCountdown > 0 && state == RED_CAPTURING) {
			digitalWrite(RED_LED_PIN, HIGH);
			delay(200);
			digitalWrite(RED_LED_PIN, LOW);
			delay(400);
			capturingCountdown--;
		}
		state = RED_AWAITING_CONNFIRM;
	}

	while (state == RED_AWAITING_CONNFIRM) {
		digitalWrite(RED_LED_PIN, LOW);
		int16_t confirmTimeout = 20;

		while (confirmTimeout > 0 && state == RED_AWAITING_CONNFIRM) {
			digitalWrite(RED_LED_PIN, HIGH);
			delay(100);
			digitalWrite(RED_LED_PIN, LOW);
			delay(100);
			confirmTimeout--;
		}
		if (state == RED_AWAITING_CONNFIRM) {
			state = NEUTRAL;
		}
	}

	while (state == RED_HAS_BASE) {
		digitalWrite(GREEN_LED_PIN, LOW);
		digitalWrite(RED_LED_PIN, HIGH);
	}
}



/*

class Runnable {
public:
	Runnable *nextRunnable;
	Runnable();
	virtual void setup() {}
	virtual void loop() {}
};

	Runnable *firstRunnable = NULL;

Runnable::Runnable() {
	nextRunnable = firstRunnable;
	firstRunnable = this;
}

void setup() {
	for (Runnable *r = firstRunnable; r; r = r->nextRunnable) {
		r->setup();
	}
}

void loop() {
	for (Runnable *r = firstRunnable; r; r = r->nextRunnable) {
		r->loop();
	}
}

class Blinker : public Runnable {
	const int pin;
	unsigned long dur;
	unsigned long ms;
public:

	Blinker(int pin, unsigned long dur) :
		pin(pin),
		dur(dur) {
	}

	void setup() {
		pinMode(pin, OUTPUT);
		ms = millis();
	}

	void loop() {
		if (millis() - ms > dur) {
			digitalWrite(pin, !digitalRead(pin));
			ms = millis();
		}
	}

	void setDuration(unsigned long newDur) {
		dur = newDur;
	}
};

class BlinkerSpeedShifter : public Runnable {
	const int analogPin;
	Blinker &blinker;

public:
	BlinkerSpeedShifter(int analogPin, Blinker &blinker) :
		analogPin(analogPin),
		blinker(blinker) {
	}

	void loop() {
		blinker.setDuration(analogRead(analogPin));
	}
};

Blinker blinkers[] = { Blinker(8, 800), Blinker(9, 900) };

BlinkerSpeedShifter element_2_shifter(0, blinkers[2]);
*/

void greenButtonISR() {

	switch (state) {
	case NEUTRAL:
		state = GREEN_CAPTURING;
		break;
	case GREEN_CAPTURING:
		break;
	case GREEN_AWAITING_CONNFIRM:
		state = GREEN_HAS_BASE;
		break;
	case GREEN_HAS_BASE:
		break;
	case RED_CAPTURING:
		state = GREEN_CAPTURING;
		break;
	case RED_AWAITING_CONNFIRM:
		state = GREEN_CAPTURING;
		break;
	case RED_HAS_BASE:
		state = GREEN_CAPTURING;
		break;
	default:
		state = NEUTRAL;
	}
}

void redButtonISR() {

	switch (state) {
	case NEUTRAL:
		state = RED_CAPTURING;
		break;
	case GREEN_CAPTURING:
		state = RED_CAPTURING;
		break;
	case GREEN_AWAITING_CONNFIRM:
		state = RED_CAPTURING;
		break;
	case GREEN_HAS_BASE:
		state = RED_CAPTURING;
		break;
	case RED_CAPTURING:
		break;
	case RED_AWAITING_CONNFIRM:
		state = RED_HAS_BASE;
		break;
	case RED_HAS_BASE:
		break;
	default:
		state = NEUTRAL;
	}

}