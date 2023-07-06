#ifndef LASER_H
#define LASER_H

#include "Arduino.h"
#include "Basics.h"

// -- The following flags can be used to fine tune the laser timing

// defines the granularity of the line interpolation. 64 means that each line is split into steps of 64 pixels in the longer direction.
// setting smaller values will slow down the rendering but will cause more linearity in the galvo movement,
// setting bigger values will cause faster rendering, but lines will not be straight anymore.
#define LASER_QUALITY 64

// Defines how long the galvos wait for the on/off toggling of the laser pointer (in microseconds), this will depend on your laser pointer.
//#define LASER_TOGGLE_DELAY 500
// Defines how long the galvos wait at the end of a line (currently only used for the 3D cube rendering, in microseconds).
//#define LASER_LINE_END_DELAY 200
// Defines the delay the laser waits after reaching a given position (in microseconds).
//#define LASER_END_DELAY 5
// Defines the delay after each laser movement (used when interpolating lines, in microseconds), if not defines, 0 is used
//#define LASER_MOVE_DELAY 5

// -- The following flags can be used to rotate/flip the output without changing the DAC wiring, just uncomment the desired swap/flip
// define this to swap X and Y on the DAC
// define this to flip along the x axis
//#define LASER_FLIP_X
// define this to flip along the y axis
#define LASER_SWAP_XY
//#define LASER_FLIP_X
#define LASER_FLIP_Y

class Laser {
public:
  Laser();
  void init(float scale, long offsetX, long offsetY);
  void sendTo(int x, int y);
  bool hardwareSelfCheck();
  void setLaserPower(byte red, byte green, byte blue);

private:
  void resetClipArea();
  void setClipArea(int x, int y, int x1, int y1);
  void sendToDAC(int x, int y);
  void sendtoRaw(int x, int y);
  int fixBoundary(int input, int min, int max);
  bool testGalvoFeedback();
  void testLaserFeedbackForWatchdog();

  const byte _redLaserPin = 2;
  const byte _greenLaserPin = 3;
  const byte _blueLaserPin = 4;
  const byte _safeOperationPin = A2;
  const byte _yGalvoFeedbackSignal = A3;
  const byte _xGalvoFeedbackSignal = A4;

  int _realTimeYPos = 0;  // The y position based on the feedback signal from the galvo
  int _realTimeXPos = 0;  // The x position based on the feedback signal from the galvo
  int _yPos = 0;
  int _xPos = 0;

  FIXPT _quality;

  long _x;
  long _y;
  int _state;

  FIXPT _scale;
  long _offsetX;
  long _offsetY;

  long _moved;
  long _maxMove;
  bool _laserForceOff;
  long _maxMoveX;
  long _maxMoveY;

  long _oldX;
  long _oldY;

  long _clipXMin;
  long _clipYMin;
  long _clipXMax;
  long _clipYMax;

  bool _enable3D;
  Matrix3 _matrix;
  long _zDist;
};

#endif