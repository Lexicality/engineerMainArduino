#include <FastLED.h>
#include <ClickEncoder.h>
#include <TimerOne.h>

#include "leds.h"
#include "encoders.h"
#include "switches.h"


#define PIN_AIRLOCK 16

// --------------- game logic bits ----------
#define STATE_DEAD -1
#define STATE_OFF 0
#define STATE_WARMUP 1
#define STATE_POWERING 2
#define STATE_PREON 3
#define STATE_ON 4

// reactor puzzle game state
int gameState = STATE_OFF;

// analog bits
int lastA = -1;
int lastB = -1;

// airlock switch state
byte lastAirlock = 0;
long lastAirlockRead = 0;
boolean lockSent = false;

// serial handling
char buffer[10]; // serial buffer
byte bufPtr = 0;

int damageTimer = 0;

void setup() {
  setupLEDs();

  pinMode(PIN_AIRLOCK, INPUT);
  digitalWrite(PIN_AIRLOCK, HIGH);

  setupSwitches();

  // serial shit
  Serial.begin(9600);
  reset();

  setupEncoders();
}

void reset() {
  resetLEDs();
  resetSwitches();

  *ledReactor = BrightRed;

  gameState = STATE_OFF;
}

void readAnalog() {
  // do analog
  int valA = 11 - map(analogRead(4), 0, 1024, 10, 0);
  int valB = 11 - map(analogRead(5), 0, 1024, 10, 0);

  if (valA != lastA) {
    lastA = valA;
    Serial.print("B");
    Serial.print(valA);
    Serial.print(",");
  }
  if (valB != lastB) {
    lastB = valB;
    Serial.print("A");
    Serial.print(valB);
    Serial.print(",");
  }
}

void processBuffer() {
  char c = buffer[0];
  if ( c == 'r') {        // reset the game states
    reset();
  }
  else if (c == 'k') {   // ship was killed, turn off all the bling
    gameState = STATE_DEAD;
    for (int i = 0; i < NUM_PANEL_LEDS; i++) {
      ledsPanel[i] = CRGB::Black;
    }
  }
  else if (c == 'D') {   // set dial value, format is D<dial num + 65><value + 65>
    // +65 is so that its readable in serial monitor A=0, B=1 etc
  }
  else if (c == 'd') { // damage
    damageTimer = 60;
  }
}

void loop() {
  // memset(leds, 0, NUMPIXELS * 3);
  readEncoders();
  updatePowerRings();

  // -------------------- serial reads --------------
  while ( Serial.available() > 0) {  // If data is available,
    char c = Serial.read();
    if (c == ',') {
      processBuffer();
      bufPtr = 0;
    }
    else {
      buffer[bufPtr] = c;
      if (bufPtr + 1 < 5) {
        bufPtr++;
      }
      else {
        bufPtr = 0;
      }
    }
  }

  if (gameState != STATE_DEAD) {
    readSwitches();
    readAnalog();

    if (lastAirlockRead + 10 < millis()) {
      lastAirlockRead = millis();
      byte b = digitalRead(PIN_AIRLOCK);
      if ((b == lastAirlock)) {
        if (b == 0) {
          if (!lockSent) {
            Serial.print("L,");
            lockSent = true;
          }
        }
        else {
          lockSent = false;
        }
      }
      lastAirlock = b;
    }

    if (damageTimer > 0) {
      damageTimer --;
    }

  }

  LEDBlinkThink();

  if (gameState == STATE_DEAD) {
    stateDead();
  }
  else if (gameState == STATE_OFF) {      // -----ship is off-------
    stateOff();
  }
  else if (gameState == STATE_WARMUP) {
    stateWarmup();
  }
  else if (gameState == STATE_POWERING) {
    statePowering();
  }
  else if (gameState == STATE_PREON) {
    statePreOn();
  }
  else if (gameState == STATE_ON) {
    stateOn();
  }

  // flicker everything if the damage timer is > 0
  if (damageTimer > 0) {
    for (int i = 0; i < NUMPIXELS; i++) {
      if (random(255) > 128) {
        leds[i] = CRGB::Black;
      }
    }
  }

  // leds
  showLEDs();
}
