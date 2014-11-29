// on

void setStateOn() {
  for (int i = 0; i < NUM_PANEL_LEDS; i++) {
    ledsPanel[i] = BrightGreen;
  }
  // power light
  *ledReactor = BrightGreen;
  // Power rings
  ringLightState = true;
}

// On state, set everything to green
void stateOn() {
  // for (int i = 0; i < NUM_PANEL_LEDS; i++) {
  //   if (switches[i] != SWITCH_POS_UP) {
  //     reset();
  //     return
  //   }
  // }

  // if someone turns reactor switch off then
  if (switches[REACTOR_SWITCH] != REACTOR_SWITCH_ON) {
    reset();
  }
}
