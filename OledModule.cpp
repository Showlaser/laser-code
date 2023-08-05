#include "OledModule.h"
#include <Encoder.h>

Encoder myEnc(5, 6);

/**
 @brief Checks for the input on the rotary encoder and changes the menu on the oled screen
*/
void OledModule::checkForInput() {
  currentSelectedMenuId = myEnc.read() / 4;
  int reading = digitalRead(8);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == 1) {
        Serial.println("Button pressed");
      }
    }
  }

  lastButtonState = reading;
}