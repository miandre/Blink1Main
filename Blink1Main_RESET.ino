#include "HeaderFiles/IOFunctions.h"

#include <Adafruit_FONA.h>
#include <SoftwareSerial.h>

#define ID 3
#define APN "halebop.telia.se"

#define BLUE "BLUE"
#define RED "RED"
#define NO_TEAM "BLACK"
#define CAPTURE_TIMEOUT 10
#define CONFIRM_TIMEOUT 10
#define RED_BUTTON_PIN 2
#define BLUE_BUTTON_PIN 3
#define RED_LED_PIN 9
#define BLUE_LED_PIN 10
#define STATUS_CAPTURING 1
#define STATUS_HOLDING 2
#define FONA_RX 4
#define FONA_TX 5
#define FONA_RST 6
#define STATUS_LED_1 19
#define STATUS_LED_2 16
#define STATUS_LED_3 13

void initFONA();
void trySendData(const String&);
void setStatus(const String&, uint8_t);
void setAlive();
void reInitGPRS();
void blueButtonISR();
void redButtonISR();
boolean sendData(const String&);

enum State {
	NEUTRAL,
	BLUE_CAPTURING,
	BLUE_AWAITING_CONNFIRM,
	BLUE_HAS_BASE,
	RED_CAPTURING,
	RED_AWAITING_CONNFIRM,
	RED_HAS_BASE
};

// this is a large buffer for replies
char replybuffer[255];

SoftwareSerial fonaSs = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSs;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

volatile State state;
boolean statusHasChanged = false;
volatile boolean buttonActive;
char PIN[5] = "1234";
const String URL_BASE = "http://www.geeks.terminalprospect.com/AIR/";
long timer = 0;
uint8_t buttonState;
uint8_t lastButtonState;


void setup() {

	pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
	pinMode(BLUE_BUTTON_PIN, INPUT_PULLUP);
	pinMode(BLUE_LED_PIN, OUTPUT);
	pinMode(RED_LED_PIN, OUTPUT);
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(PIN_A2, OUTPUT);
	pinMode(PIN_A5, OUTPUT);
	attachInterrupt(digitalPinToInterrupt(BLUE_BUTTON_PIN), blueButtonISR, RISING);
	attachInterrupt(digitalPinToInterrupt(RED_BUTTON_PIN), redButtonISR, FALLING);

	//Use with USB serial connection only
	while (!Serial);

	Serial.begin(115200);
	Serial.println(F("Initializing....(May take 3 seconds)"));
	buttonActive = true;
	initFONA();
}



void loop() {
	Serial.flush();
	if (++timer >= 4000000 && !statusHasChanged) {
		captureBlink(LED_BUILTIN);
		setAlive();
		timer = 0;
	}

	int16_t capturingCountdown = 10;
	//int16_t capturingCountdown = 550;
	int16_t confirmTimeout = 120;

	switch (state) {
	case  NEUTRAL:
		buttonActive = true;
		if (statusHasChanged) { setStatus(NO_TEAM, STATUS_HOLDING); }
		digitalWrite(RED_LED_PIN, LOW);
		digitalWrite(BLUE_LED_PIN, LOW);
		break;

	case BLUE_CAPTURING:
		digitalWrite(RED_LED_PIN, LOW);
		digitalWrite(BLUE_LED_PIN, HIGH);
		if (statusHasChanged) { setStatus(BLUE, STATUS_CAPTURING); }
		while (capturingCountdown > 0 && state == BLUE_CAPTURING) {
			capturePulse(BLUE_LED_PIN);
			capturingCountdown--;
		}
		if (!statusHasChanged) {
			state = BLUE_AWAITING_CONNFIRM;
		}
		break;

	case BLUE_AWAITING_CONNFIRM:
		digitalWrite(RED_LED_PIN, LOW);
		while (confirmTimeout > 0 && state == BLUE_AWAITING_CONNFIRM) {
			confirmBlink(BLUE_LED_PIN);
			confirmTimeout--;
		}
		if (state == BLUE_AWAITING_CONNFIRM) {
			state = NEUTRAL;
			statusHasChanged = true;
		}
		break;

	case BLUE_HAS_BASE:
		digitalWrite(RED_LED_PIN, LOW);
		digitalWrite(BLUE_LED_PIN, HIGH);
		if (statusHasChanged) { setStatus(BLUE, STATUS_HOLDING); }
		break;

	case RED_CAPTURING:
		digitalWrite(BLUE_LED_PIN, LOW);
		digitalWrite(RED_LED_PIN, HIGH);
		if (statusHasChanged) { setStatus(RED, STATUS_CAPTURING); }
		while (capturingCountdown > 0 && state == RED_CAPTURING) {
			capturePulse(RED_LED_PIN);
			capturingCountdown--;
		}
		if (!statusHasChanged) {
			state = RED_AWAITING_CONNFIRM;
		}
		break;

	case RED_AWAITING_CONNFIRM:
		digitalWrite(RED_LED_PIN, LOW);
		while (confirmTimeout > 0 && state == RED_AWAITING_CONNFIRM) {
			confirmBlink(RED_LED_PIN);
			confirmTimeout--;
		}
		if (state == RED_AWAITING_CONNFIRM) {
			state = NEUTRAL;
			statusHasChanged = true;
		}
		break;

	case RED_HAS_BASE:
		digitalWrite(BLUE_LED_PIN, LOW);
		digitalWrite(RED_LED_PIN, HIGH);
		if (statusHasChanged) { setStatus(RED, STATUS_HOLDING); }
		break;
	}
}



void flushFONA() {
	flushSerial();
	while (fona.available()) {
		Serial.write(fona.read());
	}
	fona.flush();
}

void initFONA() {

	digitalWrite(STATUS_LED_1, LOW);
	digitalWrite(STATUS_LED_2, LOW);
	digitalWrite(STATUS_LED_3, LOW);

	fonaSerial->begin(4800);
	if (!fona.begin(*fonaSerial)) {
		//Serial.println(F("Couldn't find FONA"));
		while (true);
	}

	// Optionally configure a GPRS APN, username, and password.
	fona.setGPRSNetworkSettings(F(APN));

	captureBlink(STATUS_LED_1);
	
	flushFONA();
	while (!fona.unlockSIM(PIN)) {
		captureBlink(STATUS_LED_1);
		delay(100);
		if (fona.getNetworkStatus() == 1) break;
	}

	flushFONA();
	digitalWrite(STATUS_LED_1, HIGH);

	delay(500);
	uint8_t counter = 0;
	captureBlink(STATUS_LED_2);
	while (fona.getNetworkStatus() != 1) {
		captureBlink(STATUS_LED_2);
		fona.flush();
		delay(500);
		counter++;
		if (counter == 5) {
			counter = 0;
		}
	}

	flushFONA();
	digitalWrite(STATUS_LED_2, HIGH);
	fona.enableGPRS(false);
	delay(100);
	captureBlink(STATUS_LED_3);
	while (!fona.enableGPRS(true)) {
		captureBlink(STATUS_LED_3);
		delay(200);
	}
	flushFONA();
	digitalWrite(STATUS_LED_3, HIGH);

	//buttonActive = true;
}

void reInitGPRS() {
	fona.enableGPRS(false);
	delay(100);
	fona.enableGPRS(true);
	delay(200);
}

void setStatus(const String& team, uint8_t status) {
	const String url = URL_BASE + "UpdateStatus.php?ID=" + ID + "&TEAM=" + team + "&STATUS=" + status;
	trySendData(url);
	Serial.println("loop ");
	replybuffer[10] = 'F';
	for (int j = 0; j < 11; j++) {
		Serial.println(replybuffer[j]);
	}
}

void setAlive() {
	const String url = URL_BASE + "watchDog.php?ID=" + ID;
	trySendData(url);
}

void trySendData(const String& url) {
	//buttonActive = true;
	fona.enableGPRS(true);
	delay(200);

	uint8_t reInitCounter = 5;

	while (!sendData(url)) {
		captureBlink(LED_BUILTIN);
		reInitGPRS();
		if (--reInitCounter == 0) {
			initFONA();
			delay(1000);
			reInitCounter = 5;
		}
	}

	digitalWrite(LED_BUILTIN, HIGH);
	fona.enableGPRS(false);
}


boolean sendData(const String& url) {
	uint16_t statuscode;
	int16_t length;
	uint8_t i = 0;

	char urlToSend[90];
	url.toCharArray(urlToSend, 90);
	delay(20);

	fona.flush();

	if (!fona.HTTP_GET_start(urlToSend, &statuscode, reinterpret_cast<uint16_t *>(&length))) {
		Serial.println("Failed! sendding");
		fona.flush();
		return false;
	}

	while (length > 0) {
		while (fona.available()) {
			replybuffer[i++] = fona.read();

			// Serial.write is too slow, we'll write directly to Serial register!
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
			loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
			UDR0 = replybuffer[i - 1];
#else
			Serial.write(c);
#endif
			length--;
			if (!length) break;
		}
	}
	Serial.println(F("\n****"));
	fona.HTTP_GET_end();

	fona.flush();
	statusHasChanged = false;

	return true;

}

/*****************************/
/*************ISRFOOOO************/
/****************************/

void blueButtonISR() {
	static unsigned long last_interrupt_time = 0;
	unsigned long interrupt_time = millis();

	if (interrupt_time - last_interrupt_time > 300) {
		digitalWrite(BLUE_LED_PIN, HIGH);
		digitalWrite(RED_LED_PIN, LOW);
		//Serial.print("#");



		int i = 0;
		while (digitalRead(BLUE_BUTTON_PIN) == HIGH) {
			if (++i > 35000) {
				loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
				UDR0 = '#';
				//Serial.print("*");
				confirmBlink(RED_LED_PIN);
				confirmBlink(RED_LED_PIN);
				confirmBlink(RED_LED_PIN);
				digitalWrite(BLUE_LED_PIN, LOW);
				state = NEUTRAL;
				statusHasChanged = true;

				last_interrupt_time = interrupt_time;
				//break;
			}
			else {
				//buttonActive = true;
				switch (state) {
				case NEUTRAL:
					state = BLUE_CAPTURING;
					statusHasChanged = true;
					break;
				case BLUE_CAPTURING:
					break;
				case BLUE_AWAITING_CONNFIRM:
					state = BLUE_HAS_BASE;
					statusHasChanged = true;
					break;
				case BLUE_HAS_BASE:
					break;
				case RED_CAPTURING:
					state = BLUE_CAPTURING;
					statusHasChanged = true;
					break;
				case RED_AWAITING_CONNFIRM:
					state = BLUE_CAPTURING;
					statusHasChanged = true;
					break;
				case RED_HAS_BASE:
					state = BLUE_CAPTURING;
					statusHasChanged = true;
					break;
				default:
					state = NEUTRAL;
					statusHasChanged = true;
				}
			}
		}
		i = 0;
	}
	last_interrupt_time = interrupt_time;
}

void redButtonISR() {
	digitalWrite(BLUE_LED_PIN, LOW);
	digitalWrite(RED_LED_PIN, HIGH);
	switch (state) {
	case NEUTRAL:
		state = RED_CAPTURING;
		statusHasChanged = true;
		break;
	case BLUE_CAPTURING:
		state = RED_CAPTURING;
		statusHasChanged = true;
		break;
	case BLUE_AWAITING_CONNFIRM:
		state = RED_CAPTURING;
		statusHasChanged = true;
		break;
	case BLUE_HAS_BASE:
		state = RED_CAPTURING;
		statusHasChanged = true;
		break;
	case RED_CAPTURING:
		break;
	case RED_AWAITING_CONNFIRM:
		state = RED_HAS_BASE;
		statusHasChanged = true;
		break;
	case RED_HAS_BASE:
		break;
	default:
		state = NEUTRAL;
		statusHasChanged = true;
	}
}
