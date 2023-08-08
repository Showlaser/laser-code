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
void OledModule::init(settingsModel &settings) {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  _settingsModel = settings;
  showMenu(false);
}

/**
 @brief Shows the menu on the OLED screen
*/
void OledModule::showMenu(bool buttonPressed) {
  switch (_currentSelectedMenuId) {
    case 0:
      showMainMenu(buttonPressed);
      break;
    case 1:
      showControllerIpMenu(buttonPressed);
      break;
    case 2:
      // Brightness
      break;
    default:
      break;
  }
}

/**
 @brief Sets the bottom message to be displayed
*/
void OledModule::setBottomMessage(String message) {
  _bottomMessage = message;
}

/**
 @brief Draws the top part of the menu with the provided name and the bottom part if the message is not empty and the user is on the main menu
*/
void OledModule::drawTopAndBottomOfMenu(String menuName) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(menuName);
  display.drawLine(0, 8, 127, 8, SSD1306_WHITE);

  bool bottomMessageShouldBeDisplayed = _bottomMessage != "" && menuName == "Menu";
  if (bottomMessageShouldBeDisplayed) {
    display.drawLine(0, 54, 127, 54, SSD1306_WHITE);
    display.setCursor(0, 56);
    display.println(_bottomMessage);
  }
}

/**
 @brief Shows the main menu on the oled screen
*/
void OledModule::showMainMenu(bool buttonPressed) {
  struct mainMenuItem {
    unsigned short yPosition;
    String itemName;
  };

  mainMenuItem items[3] = { { 16, "Controller IP" }, { 28, "Max laser power" }, { 40, "Brightness" } };
  _menuItemsCount = sizeof(items) / sizeof(items[0]);

  if (buttonPressed) {
    unsigned short cursorYLocation = 16 + (_currentSelectedMenuItemId * 12);
    for (unsigned short i = 0; i < _menuItemsCount; i++) {
      mainMenuItem item = items[i];
      if (cursorYLocation == item.yPosition) {
        _currentSelectedMenuId = i + 1;
        _currentSelectedMenuItemId = 0;
        showMenu(false);
        return;
      }
    }
  }

  drawTopAndBottomOfMenu("Menu");

  for (int i = 0; i < _menuItemsCount; i++) {
    mainMenuItem item = items[i];
    display.setCursor(8, item.yPosition);
    display.println(item.itemName);
  }

  display.fillCircle(3, 19 + (_currentSelectedMenuItemId * 12), 2, WHITE);
  display.display();
}

/**
 @brief Renders the controller IP menu
*/
void OledModule::showControllerIpMenu(bool buttonPressed) {
  bool userWantsToEditValue = buttonPressed && _currentSelectedMenuItemValue == -999;
  if (userWantsToEditValue) {
    Serial.println("Wants to edit");
    _currentSelectedMenuItemValue = _settingsModel.controllerIp[_currentSelectedMenuItemId];
  }

  bool userIsEditingValue = !buttonPressed && _currentSelectedMenuItemValue != -999;
  if (userIsEditingValue) {
    if (_currentSelectedMenuItemValue >= 0 && _currentSelectedMenuItemValue <= 255) {
      Serial.println("Is editing");
      _settingsModel.controllerIp[_currentSelectedMenuItemId] = _currentSelectedMenuItemValue;
    }
  }

  bool userWantsToSaveValue = buttonPressed && !userWantsToEditValue && !userIsEditingValue;
  if (userWantsToSaveValue) {
    Serial.println("Wants to save changes");
    _currentSelectedMenuItemValue = -999;
  }

  bool userPressedExit = buttonPressed && _currentSelectedMenuItemId == 4;
  if (userPressedExit) {
    _currentSelectedMenuId = 0;
    _currentSelectedMenuItemId = 0;
    _currentSelectedMenuItemValue = -999;
    return;
  }

  _menuItemsCount = 5;
  drawTopAndBottomOfMenu("Controller IP");
  for (unsigned short i = 0; i < 4; i++) {
    display.setCursor(i * 30 + 8, 28);
    display.println(_settingsModel.controllerIp[i]);
  }

  display.setCursor(52, 46);
  display.println("Exit");

  if (_currentSelectedMenuItemId == 4) {
    display.fillCircle(58, 56, 2, WHITE);
  } else {
    display.fillCircle(_currentSelectedMenuItemId * 30 + 12, 38, 2, WHITE);
  }

  display.display();
}

/**
 @brief Returns true if the button on the rotary encoder is pressed false if not pressed
*/
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

/**
 @brief Checks for the input on the rotary encoder and changes the menu on the oled screen
*/
void OledModule::checkForInput() {
  int currentReading = myEnc.read() / 4;
  bool buttonPressed = checkForButtonPress();

  if (_currentSelectedMenuItemValue != -999) {
    if (_currentSelectedMenuItemValue != currentReading) {
      _currentSelectedMenuItemValue = currentReading;
      showMenu(buttonPressed);
    }
    if (buttonPressed) {
      showMenu(buttonPressed);
    }

    return;
  }

  if (currentReading >= 0 && currentReading <= _menuItemsCount - 1) {
    if (currentReading != _currentSelectedMenuItemId) {
      _currentSelectedMenuItemId = currentReading;
      showMenu(buttonPressed);
    }
    if (buttonPressed) {
      showMenu(buttonPressed);
    }
  } else {
    myEnc.write(0);
  }
}