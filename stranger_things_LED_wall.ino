#include <Adafruit_NeoPixel.h>

#define LED_OUTPUT 0    // Output pin that the Neo Pixel LEDs are attached to
#define BTN_INPUT 2     // Input pin that button is attached to
#define NUM_LEDS 26     // Total number of LEDs (this should always be 26)

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

// These messages must be uppercase with only alphabetical characters
const char* const gMessages[] = {
  "RIGHTHERE",
  "RUN",
  "FRIENDSDONTLIE"
};

const uint8_t gNumMessages = sizeof(gMessages) / sizeof(*gMessages);

enum Mode { BLINK, SEQUENCE_SINGLE, SEQUENCE_CUMULATIVE, MESSAGE };

Adafruit_NeoPixel chain = Adafruit_NeoPixel(NUM_LEDS, LED_OUTPUT);

Mode mode = MESSAGE;
uint8_t messageIndex = 0;
uint8_t index = 0;
char * message = gMessages[messageIndex];

void setup() {
    // Configure button
    pinMode(BTN_INPUT, INPUT);
    digitalWrite(BTN_INPUT, HIGH);
    
    chain.begin();
    chain.setBrightness(40);
    turnOffStrip();
}

void loop() {
    checkButton();

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

// Detect Button Press -----------------------------

bool btn_pressed = false;

void checkButton() {
    if (! digitalRead(BTN_INPUT)) {  // if the button is pressed
        if (!btn_pressed) { // button wasn't previously pressed
            btn_pressed = true;
            onButtonDown();
        }
    } else { // button is not pressed
        if (btn_pressed) {
            btn_pressed = false;
            onButtonUp();
        }
    }
}

void onButtonUp() {
    messageIndex = (messageIndex + 1) % (gNumMessages + MESSAGE);	// Increment messageIndex
    if (messageIndex < gNumMessages) {								// messageIndex corresponds to a valid message
        mode = MESSAGE;
        message = gMessages[messageIndex];						// Update message
    } else {														// messageIndex corresponds to a different mode
        mode = static_cast<Mode>(messageIndex - gNumMessages);
    }
    index = 0;	//
    turnOffStrip();
    delay(1000);
}

void onButtonDown() {
    
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

uint32_t getColorForEnum(LightColor lightColor) {
	switch(lightColor) {
    	case YELLOW: return 0xFFFF00;
    	case BLUE: return 0x0000FF;
    	case RED: return 0xFF0000;
    	case GREEN: return 0x00FF00;
    	case ORANGE: return 0xFF9A00;
		default: return 0;
	}
}

uint32_t getColorForIndex(uint8_t i) {
	return getColorForEnum(gColors[i]);
}

void turnOffStrip() {
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    chain.setPixelColor(i, 0);
  }
  chain.show();  // Initialize all pixels to 'off'
}

uint8_t getIndexForLetter(char letter) {
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
