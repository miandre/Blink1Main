/*

*/

#include <Adafruit_FONA.h>
#include <SoftwareSerial.h>

#define BLUE "BLUE"
#define RED "RED"
#define RED_BUTTON_PIN 2
#define BLUE_BUTTON_PIN 3
#define RED_LED_PIN 8
#define BLUE_LED_PIN 12
#define STATUS_CAPTURING 1
#define STATUS_HOLDING 2
#define FONA_RX 4
#define FONA_TX 5
#define FONA_RST 6


SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
// Use this for FONA 800 and 808s
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

enum State {
	NEUTRAL,
	BLUE_CAPTURING,
	BLUE_AWAITING_CONNFIRM,
	BLUE_HAS_BASE,
	RED_CAPTURING,
	RED_AWAITING_CONNFIRM,
	RED_HAS_BASE
};

volatile State state;
char replybuffer[255];
char PIN[5] = "1234";
String URL_BASE = "http://www.geeks.terminalprospect.com/AIR/UpdateStatus.php?";


void setup() {
	pinMode(RED_BUTTON_PIN, INPUT_PULLUP);
	pinMode(BLUE_BUTTON_PIN, INPUT_PULLUP);
	pinMode(BLUE_LED_PIN, OUTPUT);
	pinMode(RED_LED_PIN, OUTPUT);
	attachInterrupt(digitalPinToInterrupt(BLUE_BUTTON_PIN), blueButtonISR, CHANGE);
	attachInterrupt(digitalPinToInterrupt(RED_BUTTON_PIN), redButtonISR, CHANGE);
	pinMode(LED_BUILTIN, OUTPUT);
	while (!Serial);

	Serial.begin(115200);
	Serial.println(F("FONA basic test"));
	Serial.println(F("Initializing....(May take 3 seconds)"));

	fonaSerial->begin(4800);
	if (!fona.begin(*fonaSerial)) {
		Serial.println(F("Couldn't find FONA"));
		while (1);
	}

	Serial.println(F("FONA is OK"));

	fona.setGPRSNetworkSettings(F("online.telia.se"));


	if (isSIMLocked()) {
		if (!fona.unlockSIM(PIN)) {
			Serial.println(F("PIN FAIL"));
		}
		else {
			Serial.println(F("PIN OK!"));
		}
	}
	else {
		Serial.println("Already Unlocked");
	}
	delay(5000);
	while (fona.getNetworkStatus() != 1) {
		delay(3000);
	}

	if (fona.GPRSstate() != 1) {
		fona.enableGPRS(false);
		delay(300);
	}
	if (!fona.enableGPRS(true)) {
		Serial.println(F("Failed to turn on"));
	}

	state = NEUTRAL;
}

void loop() {
	
	setStatus("BLACK", 2);
	while (state == NEUTRAL) {
		digitalWrite(RED_LED_PIN, LOW);
		digitalWrite(BLUE_LED_PIN, LOW);
	}

	while (state == BLUE_CAPTURING) {
		digitalWrite(RED_LED_PIN, LOW);
		int16_t capturingCountdown = 5;
		while (capturingCountdown > 0 && state == BLUE_CAPTURING) {
			digitalWrite(BLUE_LED_PIN, HIGH);
			delay(200);
			digitalWrite(BLUE_LED_PIN, LOW);
			delay(400);
			capturingCountdown--;
		}
		state = BLUE_AWAITING_CONNFIRM;
	}

	while (state == BLUE_AWAITING_CONNFIRM) {
		digitalWrite(RED_LED_PIN, LOW);
		int16_t confirmTimeout = 20;

		while (confirmTimeout > 0 && state == BLUE_AWAITING_CONNFIRM) {
			digitalWrite(BLUE_LED_PIN, HIGH);
			delay(100);
			digitalWrite(BLUE_LED_PIN, LOW);
			delay(100);
			confirmTimeout--;
		}
		if (state == BLUE_AWAITING_CONNFIRM) {
			state = NEUTRAL;
		}
	}

	while (state == BLUE_HAS_BASE) {
		digitalWrite(RED_LED_PIN, LOW);
		digitalWrite(BLUE_LED_PIN, HIGH);
	}

	while (state == RED_CAPTURING) {
		digitalWrite(BLUE_LED_PIN, LOW);
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
		digitalWrite(BLUE_LED_PIN, LOW);
		digitalWrite(RED_LED_PIN, HIGH);
	}
}

void flushSerial() {
	while (Serial.available())
		Serial.read();
}

boolean isSIMLocked() {

	String response;
	sendATCommand("AT+CPIN?");
	int i = 0;
	while (i < 255) {
		if (fona.available()) {
			response.concat((char)fona.read());
		}
		i++;
	}
	if (response.indexOf("READY") == -1) {
		return true;
	}
	else {
		return false;
	}
}

void setStatus(String team, uint8_t status) {
	uint16_t statuscode;
	int16_t length;
	String completeUrl = URL_BASE + "ID=" + 1 + "&TEAM=" + team + "&STATUS=" + status;
	char urlToSend[90]; completeUrl.toCharArray(urlToSend, 90);

	if (!fona.HTTP_GET_start(urlToSend, &statuscode, (uint16_t *)&length)) {
		Serial.println("Failed!");
	}
	else {
		Serial.println(F("Status Updated!"));
		fona.HTTP_GET_end();
	}
}

void blueButtonISR() {

	switch (state) {
	case NEUTRAL:
		state = BLUE_CAPTURING;
		break;
	case BLUE_CAPTURING:
		break;
	case BLUE_AWAITING_CONNFIRM:
		state = BLUE_HAS_BASE;
		break;
	case BLUE_HAS_BASE:
		break;
	case RED_CAPTURING:
		state = BLUE_CAPTURING;
		break;
	case RED_AWAITING_CONNFIRM:
		state = BLUE_CAPTURING;
		break;
	case RED_HAS_BASE:
		state = BLUE_CAPTURING;
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
	case BLUE_CAPTURING:
		state = RED_CAPTURING;
		break;
	case BLUE_AWAITING_CONNFIRM:
		state = RED_CAPTURING;
		break;
	case BLUE_HAS_BASE:
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



String sendATCommand(char Command[]) { //Send an AT command and wait for a response 
	int complete = 0; // have we collected the whole response? 
	int ATtimeOut = 10000;
	String response;
	char c; //capture serial stream 
	String content; //place to save serial stream 
	unsigned long commandClock = millis(); //timeout Clock 
	fonaSS.println(Command); //Print Command 
	while (!complete && commandClock <= millis() + ATtimeOut) { //wait until the command is complete 
		while (!fonaSS.available() && commandClock <= millis() + ATtimeOut); //wait until the Serial Port is opened 
		while (fonaSS.available()) { //Collect the response 
			c = fonaSS.read(); //capture it
			if (c == 0x0A || c == 0x0D); //disregard all new lines and carrige returns (makes the String matching eaiser to do) 
			else content.concat(c); //concatonate the stream into a String 
		}
		//Serial.println(content); //Debug 
		response = content; //Save it out to a global Variable (How do you return a String from a Function?) 
		complete = 1;  //Lable as Done. 
	}
	if (complete == 1)
		return response; //Is it done? return a 1 
	else
		return "NULL"; //otherwise don't (this will trigger if the command times out)  
					   /*
					   226     Note: This function may not work perfectly...but it works pretty well. I'm not totally sure how well the timeout function works. It'll be worth testing.
					   227     Another bug is that if you send a command that returns with two responses, an OK, and then something else, it will ignore the something else and just say DONE as soon as the first response happens.
					   228     For example, HTTPACTION=0, returns with an OK when it's intiialized, then a second response when the action is complete. OR HTTPREAD does the same. That is poorly handled here, hence all the delays up above.
					   229   */
}