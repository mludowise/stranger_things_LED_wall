#include <Adafruit_NeoPixel.h>

#define LED_OUTPUT 0    // Output pin that the Neo Pixel LEDs are attached to
#define NUM_LEDS 26     // Total number of LEDs (this should always be 26)

// Index in the neopixel chain for each letter. Since the chain makes a zig-zag configuration, I–Q are in backwards order.
// Change this if your neopixel LEDs are arranged in a different order.
const uint8_t * const gCharLookupTable[] = {
    // First Row
    0,  // A
    1,  // B
    2,  // C
    3,  // D
    4,  // E
    5,  // F
    6,  // G
    7,  // H

    // Second Row
    16, // I
    15, // J
    14, // K
    13, // L
    12, // M
    11, // N
    10, // O
    9, // P
    8, // Q

    // Third Row
    17, // R
    18, // S
    19, // T
    20, // U
    21, // V
    22, // W
    23, // X
    24, // Y
    25  // Z
};

// Colors for each light in the chain. The order corresponds to the index order of the neopixels, not alphabetical.
// These colors
const uint32_t * const gColors[] = {
  // First Row
  0xFDF4FD,   // A – yellow
  0x0ABEFF,   // B – blue
  0xFF00CF,   // C – red
  0x97D7C8,   // D – green
  0x4FC2DE,   // E – blue
  0xFBD056,   // F – orange
  0xE7699B,   // G – red
  0x49B3CB,   // H – green

  // Second Row
  0xE10054,   // Q – red
  0x94EDED,   // P – green
  0xB8439A,   // O – red
  0xE2AAD0,   // N – pink
  0xFCD977,   // M – orange
  0x16E492,   // L – green
  0x56D0F1,   // K – blue
  0xDD8DCC,   // J – pink
  0x89E9F0,   // I – green

  // Third Row
  0x2BFFAA,   // R – green
  0xF8CB3D,   // S – yellow
  0xF7C04B,   // T – orange
  0x00A5FF,   // U – blue
  0xFF3A59,   // V – red
  0x42AAD2,   // W – blue
  0xF8D24E,   // X – orange
  0xFF3263,   // Y – red
  0xFF0F4B    // Z – red
};

const String gMessages[] = {
  "abcdef",
  "right here",
  "run",
  "friends don't lie"
};

#define GetIndexForLetter(letter) gCharLookupTable[(letter) - 'A'] // Assumes ascii input ranging from 'A'–'Z'
//#define GetIndexForLetter(letter) ((letter) - 'A') // Assumes ascii input ranging from 'A'–'Z'

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
    chain.setPixelColor(i, blink_on ? gColors[i] : 0);
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
  chain.setPixelColor(seq_index++, gColors[seq_index]);
  chain.show();
  delay(500);
}

// Message -----------------------------------------
uint8_t msg_index = 0;
const char msg_string[] = "ABCDE\0"; 

void testMsg() {
    chain.setPixelColor(strlen(msg_string), 0xFF0000);
    chain.show();
    delay(1000);
    chain.setPixelColor(strlen(msg_string), 0);
    chain.show();
    delay(1000);
}

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
    chain.setPixelColor(index, gColors[index]);
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
