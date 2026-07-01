#ifndef LASER_H
#define LASER_H

#include "Watchdog_t4.h"
#include "Arduino.h"
#include "Settings.h"

class Laser {
public:
  void init(WDT_T4<WDT1> &watchdog);
  void sendTo(int x, int y);
  void setLaserPower(byte red, byte green, byte blue);

  // --- Realtime output path (safe to call from the IntervalTimer ISR) ---
  // Applies the projection-zone mapping to a logical (-4000..4000) coordinate.
  // Reads settings, so call from the (non-realtime) producer, not the ISR.
  void projectionMap(int &x, int &y);
  // Writes an already-projection-mapped logical (-4000..4000) coordinate
  // straight to the galvo DAC. No smoothing, no settings read, no watchdog feed.
  void writeGalvoRaw(int x, int y);
  // The power-limiting clamp + RGB DAC write, WITHOUT the settle delay.
  // setLaserPower() == applyLaserPower() + the 5us settle. The ISR relies on
  // the tick spacing for settling instead, so it calls this directly.
  void applyLaserPower(byte red, byte green, byte blue);

  void disableLasers();
  void enableLasers();
  bool testGalvoFeedback();

private:
  WDT_T4<WDT1> _watchdog;

  int fixBoundary(int input, int min, int max);
  // Both galvo axes are inverted relative to the show/frontend convention
  // (+x = right, +y = up): higher DAC voltage deflects left / down. These map a
  // logical (-4000..4000) coordinate to its inverted DAC voltage so the
  // projection matches the frontend in orientation and rotation direction.
  int mapXToVoltage(int x);
  int mapYToVoltage(int y);
  void configureDacs();

  const byte _yGalvoFeedbackSignal = A2;
  const byte _xGalvoFeedbackSignal = A3;

  int _realTimeYPos = 0;  // The y position based on the feedback signal from the galvo
  int _realTimeXPos = 0;  // The x position based on the feedback signal from the galvo
  int _yPos = 0;
  int _xPos = 0;

  bool _laserOutputDisabled = false;
};

#endif