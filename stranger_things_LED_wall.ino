#include <Adafruit_NeoPixel.h>

#define LED_OUTPUT 1    // Output pin that the Neo Pixel LEDs are attached to
#define NUM_LEDS 26     // Total number of LEDs (this should always be 26)

const uint32_t yellow = 0xFFFF00;
const uint32_t blue = 0x0000FF;
const uint32_t red = 0xFF0000;
const uint32_t green = 0x00FF00;
const uint32_t orange = 0xFF9A00;

// Colors for each light in the chain. The order corresponds to the index order of the neopixels, not alphabetical.
// These colors
const uint32_t* const gColors[] = {
  // First Row
  &yellow,  // A
  &blue,    // B
  &red,     // C
  &green,   // D
  &blue,    // E
  &orange,  // F
  &red,     // G
  &green,   // H

  // Second Row
  &red,     // Q
  &green,   // P
  &red,     // O
  &red,     // N
  &orange,  // M
  &green,   // L
  &blue,    // K
  &red,     // J
  &green,   // I

  // Third Row
  &green,   // R
  &yellow,  // S
  &orange,  // T
  &blue,    // U
  &red,     // V
  &blue,    // W
  &orange,  // X
  &red,     // Y
  &red      // Z
};

const String gMessages[] = {
  "abcdef",
  "right here",
  "run",
  "friends don't lie"
};

// Assumes ascii input ranging from 'A'–'Z'
//#define GetIndexForLetter(letter) (((letter) < 'I') ? ((letter) - 'A') : ((letter) - 'A')) // For LEDs in zig-zag order where I–Q is reversed
//#define GetIndexForLetter(letter) (((letter) >= 'I' && (letter) <= 'Q') ? (32 - (letter) + 'A') : ((letter) - 'A')) // For LEDs in zig-zag order where I–Q is reversed
//#define GetIndexForLetter(letter) ((letter) - 'A') // For LEDs in order A to Z

enum Mode { BLINK, SEQUENCE_SINGLE, SEQUENCE_CUMULATIVE, MESSAGE };

Adafruit_NeoPixel chain = Adafruit_NeoPixel(NUM_LEDS, LED_OUTPUT);

Mode mode = MESSAGE;

void setup() {
  chain.begin();
  chain.setBrightness(40);
  turnOffStrip();
  //    initMsg(0);
}

void loop() {
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

// Blink Mode --------------------------------------

bool blink_on = false;

void doBlink() {
  blink_on = !blink_on;
  for (int i = 0; i < NUM_LEDS; i++) {
    chain.setPixelColor(i, blink_on ? *gColors[i] : 0);
  }
  chain.show();
  delay(500);
}

// Sequence Mode -----------------------------------

uint8_t seq_index = 0;

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
    delay(1000);
  } else {
    doSequence();
  }
}

void doSequence() {
  chain.setPixelColor(seq_index++, *gColors[seq_index]);
  chain.show();
  delay(500);
}

// Message -----------------------------------------
uint8_t msg_index = 0;
const char* msg_string = "ABCDEFGHIJKLMNOPQRSTUVWXYZZYXWVUTSRQPONMLKJIHGFEDCBAABCDEFGHQPONMLKJRST";// Max chars 71

void initMsg(uint8_t index) {
//  String input = gMessages[index];
//  input.toUpperCase();
//  msg_string = "";
//  for (char& c : input) {
//    if (c >= 'A' && c <= 'Z') {
//      msg_string += c;
//    }
//  }
}

void doMessage() {
  if (msg_index >= strlen(msg_string)) {
    // Done with message, turn everything off for 1 second
    turnOffStrip();
    msg_index = 0;
    delay(1000);
  } else {
    if (msg_index > 0) {
      // Turn off previous letter
      chain.setPixelColor(GetIndexForLetter(msg_string[msg_index - 1]), 0); 
    }

    // Turn on this letter
    uint8_t index = GetIndexForLetter(msg_string[msg_index++]);
    chain.setPixelColor(index, *gColors[index]);
    chain.show();
    delay(1000);
  }
}

// Utils -------------------------------------------

void reset() {
  turnOffStrip();
  blink_on = false;
  seq_index = 0;
  msg_index = 0;
}

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
