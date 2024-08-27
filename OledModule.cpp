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
  int buttonPressReading = digitalRead(8);
  if (buttonPressReading != _lastButtonState) {
    _lastDebounceTime = millis();
  }

  _lastButtonState = buttonPressReading;
  if ((millis() - _lastDebounceTime) > _debounceDelay) {
    if (buttonPressReading != _buttonState) {
      _buttonState = buttonPressReading;
      if (_buttonState == 1) {
        return true;
      }
    }
  }

  return false;
}

int OledModule::getRotaryEncoderRotation() {
  int currentReading = myEnc.read();
  if (currentReading > _previousReading) {
    _previousReading = currentReading;
    return 1;
  }
  if (currentReading < _previousReading) {
    _previousReading = currentReading;
    return -1;
  }

  _previousReading = currentReading;
  return 0;
}

void OledModule::displaySelectableMenuItems(String selectableMenuItems[], String itemToShowCursorAt) {
  const int spaceBetweenItemsPx = 12;
  const int maxItemsToDisplay = 4;

  int menuItemsLength = sizeof(selectableMenuItems) / sizeof(selectableMenuItems[0]);
  int itemsToIterateOverCount = menuItemsLength > maxItemsToDisplay ? maxItemsToDisplay : menuItemsLength;

  for (int i = 0; i < itemsToIterateOverCount; i++) {
    println(8, i * 12, selectableMenuItems[i]);
    if (selectableMenuItems[i] == itemToShowCursorAt) {
      printCircle(3, i * 12);
    }
  }
}