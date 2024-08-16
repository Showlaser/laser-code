#ifndef LASER_H
#define LASER_H

#include "Arduino.h"

class Laser
{
public:
  void init();
  void sendTo(int x, int y);
  bool hardwareSelfCheck();
  void setLaserPower(byte red, byte green, byte blue);

private:
  int fixBoundary(int input, int min, int max);
  bool testGalvoFeedback();
  void testLaserFeedbackForWatchdog();
  void configureDacs();
  int mapPositionToResolution(int value);

  const byte _safeOperationPin = A2;
  const byte _yGalvoFeedbackSignal = A3;
  const byte _xGalvoFeedbackSignal = A4;

  int _realTimeYPos = 0; // The y position based on the feedback signal from the galvo
  int _realTimeXPos = 0; // The x position based on the feedback signal from the galvo
  int _yPos = 0;
  int _xPos = 0;
};

#endif