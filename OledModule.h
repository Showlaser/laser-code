#ifndef OledModule_H
#define OledModule_H

#include "Arduino.h"
#include "Settings.h"

class OledModule {
public:
  /**
  @brief Initializes the OLED screen and displays "hardware testing" on the screen 
  */
  void init();

  /**
  @brief sets the location of the cursor by x and y
  @param x the location of the x position
  @param y the location of the y position
  */
  void setCursor(int x, int y);

  /**
  @brief prints a line at the current location of the cursor
  @param startX the start of x position of the line
  @param startY the start of y position of the line
  @param endX the end of x position of the line
  @param endY the end of y position of the line
  */
  void drawline(int startX, int startY, int endX, int endY);

  /**
  @brief prints text at the provided location of the cursor
  @param x the start of x position of the text
  @param y the start of y position of the text
  @param text the text to print
  */
  void println(int x, int y, String text);

  /**
  @brief prints a circle at the current location of the cursor
  @param x the start of x position of the line
  @param y the start of y position of the line
  */
  void printCircle(int x, int y);

  /**
  @brief clears the content on the oled screen
  */
  void clearDisplay();

  /**
  @brief reads the value of the rotary encoder button and returns it
  @param previousButtonPressed the previous value of the button press
  @param buttonPressed the current value of the button press
  */
  void checkForButtonPress(bool &previousButtonPressed, bool &buttonPressed);

  /**
  @brief resets the value of the rotary encoder and previous reading
  */
  void resetRotaryValue();

  /**
  @brief reads the value of the rotary encoder knob and returns it
  @param previousReading this parameter will be updated with the previous value of the rotary encoder
  @param currentReading this parameter will be updated with the current value of the rotary encoder
  */
  void getRotaryEncoderRotation(int &previousReading, int &currentReading);

  /**
  @brief displays a menu in which selectable items are shown. A max of 4 items can be shown on the menu. If you need more you need to select a range to display 
  @param selectableMenuItems a string array of selectable items
  @param selectableMenuItemsLength the length of the selectableMenuItems parameter
  @param selectedItem the name of the current selected item to show the cursor at
  */
  void displaySelectableMenuItems(String selectableMenuItems[], int selectableMenuItemsLength, String itemToShowCursorAt);

private:
  int _previousReading = 0;
  bool _previousButtonState = false;

  const byte _outputA = 2;
  const byte _outputB = 3;
  const byte _switch = 4;
};

#endif