#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR_ATtiny85__ // Trinket, Gemma, etc.
    #include <avr/power.h>
#endif

#define RX_PIN		2	// Connect this Trinket pin to BLE 'TXO' pin
#define CTS_PIN		1	// Connect this Trinket pin to BLE 'CTS' pin
#define LED_OUTPUT	0	// Connect NeoPixels to this Trinket pin
#define NUM_LEDS	26	// Total number of LEDs (this should always be 26)
#define MAX_MSG_LEN	20	// Maximum message size

enum LightColor { YELLOW, BLUE, RED, GREEN, ORANGE };

// Colors for each light in the chain. The order corresponds to the index order of the neopixels, not alphabetical.
// These colors
const LightColor gColors[] = {
  // First Row
  YELLOW,  // A
  BLUE,    // B
  RED,     // C
  GREEN,   // D
  BLUE,    // E
  ORANGE,  // F
  RED,     // G
  GREEN,   // H

  // Second Row
  RED,     // Q
  GREEN,   // P
  RED,     // O
  RED,     // N
  ORANGE,  // M
  GREEN,   // L
  BLUE,    // K
  RED,     // J
  GREEN,   // I

  // Third Row
  GREEN,   // R
  YELLOW,  // S
  ORANGE,  // T
  BLUE,    // U
  RED,     // V
  BLUE,    // W
  ORANGE,  // X
  RED,     // Y
  RED      // Z
};

#define GetColorForEnum(lightColor) (   \
    lightColor == YELLOW ? 0xFFFF00 :   \
    lightColor == BLUE ? 0x0000FF :     \
    lightColor == RED ? 0xFF0000 :      \
    lightColor == GREEN ? 0x00FF00 :    \
    lightColor == ORANGE ? 0xFF9A00 :   \
    0                                   \
)
#define GetColorForIndex(index) (GetColorForEnum(gColors[index]))

// These messages must be uppercase with only alphabetical characters
const char* const gMessages[] = {
  "RIGHTHERE",
  "RUN",
  "FRIENDSDONTLIE"
};

const uint8_t gNumMessages = sizeof(gMessages) / sizeof(*gMessages);

enum Mode { BLINK, SEQUENCE_SINGLE, SEQUENCE_CUMULATIVE, MESSAGE };

SoftwareSerial bt(RX_PIN, -1);
Adafruit_NeoPixel chain = Adafruit_NeoPixel(NUM_LEDS, LED_OUTPUT);

Mode mode = MESSAGE;

void setup() {
	#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
		// MUST do this on 16 MHz Trinket for serial & NeoPixels!
		clock_prescale_set(clock_div_1);
	#endif
	// Stop incoming data & init software serial
	pinMode(CTS_PIN, OUTPUT); digitalWrite(CTS_PIN, HIGH);
	bt.begin(9600);

    chain.begin();
    chain.setBrightness(40);
    turnOffStrip();
    initMsg("RUN");
    initSequence();
    initBlink();
}

void loop() {

    checkBluetooth();

//    if (! checkIfWaiting() ) {
	    doMessage();

//        switch (mode) {
//            case BLINK:
//                doBlink();
//                break;
//            case SEQUENCE_SINGLE:
//                doSequenceSingle();
//                break;
//            case SEQUENCE_CUMULATIVE:
//                doSequenceCumulative();
//                break;
//            case MESSAGE:
//                doMessage();
//                break;
//        }
//    }
}

// Blink Mode --------------------------------------

bool blink_on = false;

void initBlink() {
    blink_on = false;
}

void doBlink() {
    blink_on = !blink_on;
    for (int i = 0; i < NUM_LEDS; i++) {
        chain.setPixelColor(i, blink_on ? GetColorForIndex(i) : 0);
    }
    chain.show();
    setWait(random(100, 700));
}

// Sequence Mode -----------------------------------

uint8_t seq_index = 0;

void initSequence() {
    seq_index = 0;
}

void doSequenceSingle() {
    if (seq_index > 0) {
        // Turn off previous color
        chain.setPixelColor(seq_index - 1, 0);
    }
    seq_index %= NUM_LEDS;
    doSequence();
}

void doSequenceCumulative() {
    if (seq_index >= NUM_LEDS) {
        turnOffStrip();
        seq_index = 0;
        setWait(1000);
    } else {
        doSequence();
    }
}

void doSequence() {
    chain.setPixelColor(seq_index++, GetColorForIndex(seq_index));
    chain.show();
    setWait(500);
}

// Message -----------------------------------------
uint8_t msg_index = 0;
char msg_chars[MAX_MSG_LEN + 1] = {0};

void initMsg(char* newMsg) {
    msg_index = 0;
    
	int j=0;
    for (int i = 0; i < strlen(newMsg); i++) {
    	char c = newMsg[i];
    	if (c >= 'a' && c <= 'z') {
    		c = c + ('A' - 'a');
    	} else if (c < 'A' || c > 'Z') {
    		continue;
    	}
    	msg_chars[j++] = c;
    }

	msg_chars[j] = 0;
}

void doMessage() {
    if (msg_index > 0) {
        // Turn off previous letter
        chain.setPixelColor(GetIndexForLetter(msg_chars[msg_index - 1]), 0); 
    }
    
    if (msg_index >= strlen(msg_chars)) {
        // Done with message, turn everything off for 1 second
        msg_index = 0;
    } else {
        // Turn on this letter
        uint8_t index = GetIndexForLetter(msg_chars[msg_index++]);
        chain.setPixelColor(index, GetColorForIndex(index));
    }
    chain.show();
    setWait(1000);
}

// Bluetooth ---------------------------------------

void checkBluetooth() {
	int i = 0;
	char tempMsg[MAX_MSG_LEN + 1];
	
	// Animation happens at about 30 frames/sec.  Rendering frames takes less
	// than that, so the idle time is used to monitor incoming serial data.
	digitalWrite(CTS_PIN, LOW); // Signal to BLE, OK to send data!

	i = bt.readBytes(tempMsg, MAX_MSG_LEN);
	tempMsg[i] = 0;

	if (strlen(tempMsg) > 0) {
		flushBluetooth(); // If message exceeds MAX_MSG_LEN, flush the excess.
		initMsg(tempMsg);
		turnOffStrip();
		delay(1000);
	}
	
	digitalWrite(CTS_PIN, HIGH); // BLE STOP!
}

void flushBluetooth() {
	char c;
	while(bt.readBytes(&c, 1) > 0);
}

/* Delay Utilities ---------------------------------

    We can't use the delay() method because then we 
    won't detect the button press unless it's held 
    down throughout the duration of the current delay();

    Instead, we'll continuously loop and check if the
    intended wait time has passed to light up the LEDs.
*/
unsigned long waitUntil = 0;

void setWait(int milliseconds) {
    waitUntil = millis() + milliseconds;
}

bool checkIfWaiting() {
    return millis() < waitUntil;
}

// Utils -------------------------------------------

void turnOffStrip() {
  for (int i = 0; i < NUM_LEDS; i++) {
    chain.setPixelColor(i, 0);
  }
  chain.show();  // Initialize all pixels to 'off'
}

uint8_t GetIndexForLetter(char letter) {
    return (letter < 'I' || letter > 'Q') ? (letter - 'A') : (24 - letter + 'A');
}

void blinkNum(uint8_t num, uint32_t color) {
    chain.setPixelColor(num, color);
    chain.show();
    delay(1000);
    chain.setPixelColor(num, 0);
    chain.show();
    delay(1000);
}
