#ifndef LASER_H
#define LASER_H

#include "Watchdog_t4.h"
#include "Arduino.h"
#include "Settings.h"

class Laser
{
public:
  void init(WDT_T4<WDT1> &watchdog, settingsModel laserSettings);
  void sendTo(int x, int y);
  bool hardwareSelfCheck();
  void setLaserPower(byte red, byte green, byte blue);
  void disableLasers();
  void enableLasers();

private:
  WDT_T4<WDT1> _watchdog;
  settingsModel _laserSettings;

  int fixBoundary(int input, int min, int max);
  bool testGalvoFeedback();
  void testLaserFeedbackForWatchdog();
  void configureDacs();
  void logisticGrowthCurve(int startValue, int endValue, int duration, float growRate);

  const byte _safeOperationPin = A2;
  const byte _yGalvoFeedbackSignal = A2;
  const byte _xGalvoFeedbackSignal = A3;

  int _realTimeYPos = 0; // The y position based on the feedback signal from the galvo
  int _realTimeXPos = 0; // The x position based on the feedback signal from the galvo
  int _yPos = 0;
  int _xPos = 0;

  bool _laserOutputDisabled = false;
};

#endif