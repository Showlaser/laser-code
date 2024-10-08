#include "OledModule.h"
#include <Encoder.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Encoder myEnc(5, 6);

void OledModule::init() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3D for 128x64
    for (;;)
      ;
  }

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.display();
}

void OledModule::setCursor(int x, int y) {
  display.setCursor(x, y);
}

void OledModule::drawline(int startX, int startY, int endX, int endY) {
  display.drawLine(startX, startY, endX, endY, WHITE);
}

void OledModule::println(int x, int y, String text) {
  setCursor(x, y);
  display.println(text);
}

void OledModule::printCircle(int x, int y) {
  display.fillCircle(x, y, 2, WHITE);
}

void OledModule::clearDisplay() {
  display.clearDisplay();
}

bool OledModule::checkForButtonPress() {
  static bool lastButtonState = HIGH;  // Assume button is unpressed at start
  bool buttonState = digitalRead(8);

  if (lastButtonState == HIGH && buttonState == LOW) {
    // Button was just pressed
    _previousButtonState = true;
  } else if (lastButtonState == LOW && buttonState == HIGH) {
    // Button was just released
    if (_previousButtonState) {
      _previousButtonState = false;
      lastButtonState = buttonState;
      return true;
    }
  }

  lastButtonState = buttonState;
  return false;
}

void OledModule::overWriteRotaryValue(int value) {
  myEnc.write(value);
}

void OledModule::resetRotaryValue() {
  myEnc.write(0);
}

void OledModule::getRotaryEncoderRotation(int &previousReading, int &currentReading) {
  currentReading = myEnc.read() / 4;
  previousReading = _previousReading;
  _previousReading = currentReading;
}

void OledModule::displaySelectableMenuItems(String selectableMenuItems[], int selectableMenuItemsLength, String itemToShowCursorAt) {
  const int spaceBetweenItemsPx = 11;
  const int maxItemsToDisplay = 6;
  int itemsToIterateOverCount = selectableMenuItemsLength > maxItemsToDisplay ? maxItemsToDisplay : selectableMenuItemsLength;

  for (int i = 0; i < itemsToIterateOverCount; i++) {
    println(8, i * spaceBetweenItemsPx, selectableMenuItems[i]);
    if (selectableMenuItems[i] == itemToShowCursorAt) {
      printCircle(3, i * spaceBetweenItemsPx + 3);
    }
  }
}

void OledModule::displayChanges() {
  display.display();
}