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

// Modes - The mode the LEDs are in. MESSAGE
#define BLINK				0	// Button 1 - All LEDs blink on and off at random intervals
#define SEQUENCE_SINGLE		1	// Button 2 - One LED turns on at a time starting from the beginning of the LED chain
#define SEQUENCE_CUMULATIVE 2	// Button 3 - The LEDs turn on and stay on one at a time from the beginning of the LED chain
#define MESSAGE				3	// Button 4 - The LEDs light up to spell a message

// Light Colors - Change these to tweak colors
#define WHITE		0xFFFCCC
#define BLUE 		0x0000FF
#define RED			0xFF0057
#define GREEN		0x00FF7D
#define ORANGE		0xFFC000

// Colors for each light in the chain. The order corresponds to the index order of the neopixels, not alphabetical.
uint32_t getColorForIndex(uint8_t i) {
	switch (i) {
		case 0:		// A
		case 18:	// S
			return WHITE;
		
		case 1:		// B
		case 4:		// E
		case 14:	// K
		case 20:	// U
			return BLUE;
		
		case 2:		// C
		case 6:		// G
		case 15:	// J
		case 11:	// N
		case 10:	// O
		case 8:		// Q
		case 21:	// V
		case 24:	// Y
		case 25:	// Z
			return RED;
		
		case 3:		// D
		case 7:		// H
		case 16:	// I
		case 13:	// L
		case 9:		// P
		case 17:	// R
		case 22:	// W
			return GREEN;
		
		case 5:		// F
		case 12:	// M
		case 19:	// T
		case 23:	// X
			return ORANGE;
	}
}

// Returns the index (0-25) of the LED for the corresponding letter
uint8_t getIndexForLetter(char letter) {
	// Use this if letters are zig-zag in the chain
    return (letter < 'I' || letter > 'Q') ? (letter - 'A') : (24 - letter + 'A');

    // Use this if letters are sequential instead of zig-zag
//    return letter - 'A';
}

uint8_t mode = MESSAGE;							// Change this to set the default mode of the LEDs (must be BLINK, SEQUENCE_SINGLE, SEQUENCE_CUMULATIVE, or MESSAGE).
char message[MAX_MSG_LEN + 1] = "RIGHTHERE";	// Change this to set the default message (must be uppercase and letters only).

uint8_t index = 0;
SoftwareSerial bt = SoftwareSerial(RX_PIN, -1);
Adafruit_NeoPixel chain = Adafruit_NeoPixel(NUM_LEDS, LED_OUTPUT);

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
}

void loop() {

    checkBluetooth();

    if (! checkIfWaiting() ) {
        switch (mode) {
            case BLINK:
                doBlink();
                break;
            case SEQUENCE_SINGLE:
                doSequenceSingle();
                break;
            case SEQUENCE_CUMULATIVE:
                doSequenceCumulative();
                break;
            case MESSAGE:
                doMessage();
                break;
        }
    }
}

// Blink Mode --------------------------------------

void doBlink() {
    index = !index;
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        chain.setPixelColor(i, index ? getColorForIndex(i) : 0);
    }
    chain.show();
    setWait(random(100, 700));
}

// Sequence Mode -----------------------------------

void doSequenceSingle() {
    if (index > 0) {
        // Turn off previous color
        chain.setPixelColor(index - 1, 0);
    }
    index %= NUM_LEDS;
    doSequence();
}

void doSequenceCumulative() {
    if (index >= NUM_LEDS) {
        turnOffStrip();
        index = 0;
        setWait(1000);
    } else {
        doSequence();
    }
}

void doSequence() {
    chain.setPixelColor(index++, getColorForIndex(index));
    chain.show();
    setWait(500);
}

// Message -----------------------------------------

void cleanMsg() {
	uint8_t j=0;
    for (uint8_t i = 0; i < strlen(message); i++) {
    	char c = message[i];
    	if (c >= 'a' && c <= 'z') {
    		c = c + ('A' - 'a');
    	} else if (c < 'A' || c > 'Z') {
    		continue;
    	}
    	message[j++] = c;
    }

	message[j] = 0;
}

void doMessage() {
    if (index > 0) {
        // Turn off previous letter
        chain.setPixelColor(getIndexForLetter(message[index - 1]), 0); 
    }
    
    if (index >= strlen(message)) {
        // Done with message, turn everything off for 1 second
        index = 0;
    } else {
        // Turn on this letter
        uint8_t i = getIndexForLetter(message[index++]);
        chain.setPixelColor(i, getColorForIndex(i));
    }
    
    chain.show();
    setWait(1000);
}

// Bluetooth ---------------------------------------

void checkBluetooth() {
	char c;
	
	digitalWrite(CTS_PIN, LOW);		// Signal to BLE, OK to send data!

	if (bt.readBytes(&c, 1) <= 0) {	// If there's nothing to read, don't overwrite message
		return;
	}
	if (c == '!') {
		if (parseMode() >= 0) {
			turnOff1s();
		}
	} else {
		parseMessage(c);
		turnOff1s();
	}

	flushBluetooth();				// If message exceeds MAX_MSG_LEN, flush the excess.
	
	digitalWrite(CTS_PIN, HIGH);	// BLE STOP!
}

void turnOff1s() {
	turnOffStrip();					// Turn off the strip for 1 second to indicate a new message or mode
	delay(1000);
}

void parseMessage(char firstChar) {
	mode = MESSAGE;											// Set to MESSAGE mode
	
	message[0] = firstChar;								// Update first character of message
	uint8_t i = bt.readBytes(message + 1, MAX_MSG_LEN - 1);	// Read the rest of the message
	message[i+1] = 0;										// Set null terminator to indicate end of string
	index = 0;												// Resets message index to 0
	cleanMsg();												// Cleans up any invalid characters from message
}

uint8_t parseMode() {
	char buffer[2];

	// Expected input should be ['B']['x']['1'] for button pressed

	if (bt.readBytes(buffer, 2) < 2 			// Invalid input
		|| buffer[0] != 'B'  					// Input wasn't a button press, ignore
		|| buffer[1] < '1' || buffer[1] > '4') {	// Button wasn't 1-4, ignore
		return -1;
	}

	mode = buffer[1] - '1';						// Set's mode to raw MODE enum value
	index = 0;									// Reset index counter
	return mode;
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

void setWait(uint16_t milliseconds) {
    waitUntil = millis() + milliseconds;
}

bool checkIfWaiting() {
    return millis() < waitUntil;
}

// Utils -------------------------------------------

void turnOffStrip() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    chain.setPixelColor(i, 0);
  }
  chain.show();  // Initialize all pixels to 'off'
}

void blinkNum(uint8_t num, uint32_t color) {
    chain.setPixelColor(num, color);
    chain.show();
    delay(1000);
    chain.setPixelColor(num, 0);
    chain.show();
    delay(1000);
}
