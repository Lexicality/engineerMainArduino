#include <FastLED.h>
#include <ClickEncoder.h>
#include <TimerOne.h>

#include "leds.h"
#include "encoders.h"


#define PIN_AIRLOCK 16

#define PIN_DATA  12
#define PIN_CLOCK 3
#define PIN_LOAD  2

// main pane switches
byte switches[6];
byte lastSwitches[6];
boolean switchesChanged = false;


// map bits to switch num and pos
// {bit, switch pos }
byte switchMap[] = {
  3, 2,
  1, 0,
  7, 6,
  5, 4,
  12, 13
};

long switchPos = 0;
int lastChangedSwitch = 0;

// --------------- game logic bits ----------
#define STATE_DEAD -1
#define STATE_OFF 0
#define STATE_WARMUP 1
#define STATE_POWERING 2
#define STATE_PREON 3
#define STATE_ON 4

// reactor puzzle game state
int gameState = STATE_OFF;

byte warmupOrder[5];      // order of switches for warmup (orange) phase
byte powerOnOrder[5];     // order of switches for powerup (green) phse
byte currentSwitch = 0;   // switch id we are waiting for correct value for

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

  // power man buttons
  pinMode(5, INPUT);
  digitalWrite(5, HIGH);
  pinMode(6, INPUT);
  digitalWrite(6, HIGH);
  pinMode(7, INPUT);
  digitalWrite(7, HIGH);
  pinMode(8, INPUT);
  digitalWrite(8, HIGH);

  pinMode(PIN_AIRLOCK, INPUT);
  digitalWrite(PIN_AIRLOCK, HIGH);

  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_LOAD, OUTPUT);
  pinMode(PIN_DATA, INPUT);

  digitalWrite(PIN_CLOCK, LOW);

  // serial shit
  Serial.begin(9600);
  reset();

  setupEncoders();
}

void reset() {
  resetLEDs();

  warmupOrder[0] = 0;
  warmupOrder[1] = 2;
  warmupOrder[2] = 4;
  warmupOrder[3] = 1;
  warmupOrder[4] = 3;

  powerOnOrder[0] = 1;
  powerOnOrder[1] = 4;
  powerOnOrder[2] = 2;
  powerOnOrder[3] = 0;
  powerOnOrder[4] = 3;

  for (int i = 4; i > 0; i--) {
    int rand = random(i + 1);
    if (rand != i) {
      int t = warmupOrder[i];
      warmupOrder[i] = warmupOrder[rand];
      warmupOrder[rand] = t;
    }
  }

  for (int i = 4; i > 0; i--) {
    int rand = random(i + 1);
    if (rand != i) {
      int t = powerOnOrder[i];
      powerOnOrder[i] = powerOnOrder[rand];
      powerOnOrder[rand] = t;
    }
  }

  *ledReactor = BrightRed;

  gameState = STATE_OFF;
  currentSwitch = 0;
  Serial.print("S0,");
}

void readSwitches() {
  switchPos = 0;
  switchesChanged = false;
  digitalWrite(PIN_LOAD, LOW);
  digitalWrite(PIN_LOAD, HIGH);

  delay(5);

  for (int i = 0 ; i < 16; i++) {
    digitalWrite(PIN_CLOCK, LOW);
    byte a = digitalRead(PIN_DATA);
    switchPos |= ( (long)a << i);
    digitalWrite(PIN_CLOCK, HIGH);
  }

  for (int i = 0; i < 5; i++) {
    byte p = (switchPos & 1l << switchMap[i * 2] ) == 0 ? 0 : 1;
    byte p2 = (switchPos & 1l << switchMap[i * 2 + 1] ) == 0 ? 0 : 1;
    p2 <<= 1;
    p = p2 | p ;

    switches[i] = p;
    if (lastSwitches[i] != switches[i]) {
      lastSwitches[i] = p;
      lastChangedSwitch = i;
      //      Serial.(lastChangedSwitch);
      switchesChanged = true;
    }
  }
  switches[5] = (switchPos & 1l << 15) == 0 ? 1 : 0; // inverted for the power switch
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

/*do the ring leds*/

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
