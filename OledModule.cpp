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

/**
 @brief Initializes the OLED screen and displays "hardware testing" on the screen 
*/
void OledModule::init() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  display.clearDisplay();
  showMenu(false);
}

/**
 @brief Shows the menu on the OLED screen
*/
void OledModule::showMenu(bool buttonPressed) {
  switch (currentSelectedMenuId) {
    case 0:
      showMainMenu();
      break;
    case 1:
      // Controller IP
      break;
    case 2:
      // Brightness
      break;
    default:
      break;
  }
}

void OledModule::setBottomMessage(String message) {
  bottomMessage = message;
}

void OledModule::drawTopAndBottomOfMenu(String menuName) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(menuName);
  display.drawLine(0, 8, 127, 8, SSD1306_WHITE);

  bool bottomMessageShouldBeDisplayed = bottomMessage != "" && menuName == "Menu";
  if (bottomMessageShouldBeDisplayed) {
    display.drawLine(0, 54, 127, 54, SSD1306_WHITE);
    display.setCursor(0, 56);
    display.println(bottomMessage);
  }
}

/**
 @brief Shows the main menu on the oled screen
*/
void OledModule::showMainMenu() {
  drawTopAndBottomOfMenu("Menu");
  menuItemsCount = 2;

  display.setCursor(8, 16);
  display.println("ControllerIp");

  display.setCursor(8, 28);
  display.println("Brightness");

  display.fillCircle(3, 19 + (currentSelectedMenuItemId * 12), 2, WHITE);

  display.display();
}

/**
 @brief Checks for the input on the rotary encoder and changes the menu on the oled screen
*/
void OledModule::checkForInput() {
  int currentReading = myEnc.read() / 4;

  if (currentReading >= 0 && currentReading <= menuItemsCount - 1) {
    if (currentReading != currentSelectedMenuItemId) {
      currentSelectedMenuItemId = currentReading;
      showMenu(false);
    }
  } else {
    myEnc.write(0);
  }

  int reading = digitalRead(8);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == 1) {
        Serial.println("Button pressed");
        showMenu(true);
      }
    }
  }

  lastButtonState = reading;
}