// powering

void statePowering() {
  // clear leds
  clearLEDs();
  // set the LEDs to be the colours they should be
  for (int i = 0; i < NUM_PANEL_LEDS; ++i) {
    if (switches[i] == SWITCH_POS_MID) {
      ledsPanel[i] = PanelYellow;
    }
    else if (switches[i] == SWITCH_POS_UP) {
      ledsPanel[i] = PanelGreen;
    }
  }
  // set the reactor switch led to red
  *ledReactor = BrightRed;

  int curLight = powerOnOrder[currentSwitch];
  // ----- blink current light green
  if (blinker) {
    ledsPanel[curLight] = PanelGreen;
  }

  // made it to switch 5 without cocking up, switch to pre-on state
  if (currentSwitch == 5) {
    currentSwitch = 0;
    gameState = STATE_PREON;
    setBlinkSpeed(75);
  }
  // did the switches change? if so then set the next one in sequence to green and transmit the current switch state
  if (expectSwitchChange(curLight, SWITCH_POS_UP)) {
    currentSwitch++;
    Serial.print("S");
    Serial.print(currentSwitch + 5);
    Serial.print(",");
  }
}
