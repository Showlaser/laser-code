#include "Laser.h"
#include "math.h"
#include "Dac.h"
#include "Basics.h"

MCP4X dac;

Laser::Laser() {
  _quality = FROM_FLOAT(1. / (LASER_QUALITY));

  _x = 0;
  _y = 0;
  _oldX = 0;
  _oldY = 0;

  _state = 0;

  _scale = 1;
  _offsetX = 0;
  _offsetY = 0;

  _moved = 0;
  _maxMove = -1;
  _laserForceOff = false;
  resetClipArea();

  _enable3D = false;
  _zDist = 1000;
}

/**
 @brief Initializes the laser class

 @param scale the scale at which the laser image is projected
 @param offsetX the offset on the x axis
 @param offsetY the offset on the y axis
*/
void Laser::init(float scale, long offsetX, long offsetY) {
  _scale = FROM_FLOAT(scale);
  _offsetX = offsetX;
  _offsetY = offsetY;

  dac.init(MCP4X_4822, 5000, 5000, 10, 7, 1);
  dac.setGain2x(MCP4X_CHAN_A, 0);
  dac.setGain2x(MCP4X_CHAN_B, 0);
  dac.begin(1);
}

/**
 @brief Sends the coordinates to the DAC

 @param x the location the galvo should move on the x axis to move to
 @param y the location the galvo should move on the y axis to move to
*/
void Laser::sendToDAC(int x, int y) {
#ifdef LASER_SWAP_XY
  int x1 = y;
  int y1 = x;
#else
  int x1 = x;
  int y1 = y;
#endif
#ifdef LASER_FLIP_X
  x1 = 4095 - x1;
#endif
#ifdef LASER_FLIP_Y
  y1 = 4095 - y1;
#endif
  dac.output2(x1, y1);
}

void Laser::resetClipArea() {
  _clipXMin = 0;
  _clipYMin = 0;
  _clipXMax = 4095;
  _clipYMax = 4095;
}

void Laser::setClipArea(int x, int y, int x1, int y1) {
  _clipXMin = x;
  _clipYMin = y;
  _clipXMax = x1;
  _clipYMax = y1;
}

/**
   @brief sets the input value to the max or min value if it goes under or over the min and max values.

   @param input the input variable to fix
   @param min the minimum allowed value
   @param max the maximum allowed value
   @return int the value between or equal to the min or max value
*/
int Laser::fixBoundary(int input, int min, int max) {
  if (input < min) {
    input = min;
  }
  if (input > max) {
    max = max;
  }
  return input;
}

/**
   @brief sends the mirrors of the galvo to the specified location

   @param xpos the position of the x mirror in the galvo
   @param ypos the position of the y mirror in the galvo
*/
void Laser::sendTo(int xPos, int yPos) {
  xPos = fixBoundary(xPos, -4000, 4000);
  yPos = fixBoundary(yPos, -4000, 4000);

  int delayMicrosecondsTimeX = (_xPos > xPos ? _xPos - xPos : xPos - _xPos) / 3;
  int delayMicrosecondsTimeY = (_yPos > yPos ? _yPos - yPos : yPos - _yPos) / 3;
  int delayTime = delayMicrosecondsTimeX > delayMicrosecondsTimeY ? delayMicrosecondsTimeX : delayMicrosecondsTimeY;
  if (delayTime < 25) {
    delayTime = 25;
  }
  if (delayTime > 1000) {
    delayTime = 1000;
  }

  _xPos = xPos;
  _yPos = yPos;

  long xNew = TO_INT(_xPos * _scale) + _offsetX;
  long yNew = TO_INT(_yPos * _scale) + _offsetY;

  sendtoRaw(xNew, yNew);
  delayMicroseconds(delayTime);
}

void Laser::sendtoRaw(int xNew, int yNew) {
  // devide into equal parts, using _quality
  long fdiffx = xNew - _x;
  long fdiffy = yNew - _y;
  long diffx = TO_INT(abs(fdiffx) * _quality);
  long diffy = TO_INT(abs(fdiffy) * _quality);

  // use the bigger direction
  if (diffx < diffy) {
    diffx = diffy;
  }

  fdiffx = FROM_INT(fdiffx) / diffx;
  fdiffy = FROM_INT(fdiffy) / diffx;
  // interpolate in FIXPT
  FIXPT tmpx = 0;
  FIXPT tmpy = 0;
  for (int i = 0; i < diffx - 1; i++) {
    tmpx += fdiffx;
    tmpy += fdiffy;
    sendToDAC(_x + TO_INT(tmpx), _y + TO_INT(tmpy));
  }

  _x = xNew;
  _y = yNew;
  sendToDAC(_x, _y);
}

/**
 @brief This function sets the power of the laser by the provided values.
 @brief Values below 0 will be set to 0 and values above 255 will be set to 255

 @param red the power the red laser should output from 0 / 255
 @param green the power the green laser should output from 0 / 255
 @param blue the power the blue laser should output from 0 / 255
*/
void setLaserPower(byte red, byte green, byte blue) {
  r = fixBoundary(r, 0, 255);
  g = fixBoundary(g, 0, 255);
  b = fixBoundary(b, 0, 255);

  analogWrite(_redLaserPin, r);
  analogWrite(_greenLaserPin, g);
  analogWrite(_blueLaserPin, b);
}

/**
 @brief This function checks if the feedback of the galvo's is working by sending them to their maximums positions and reading the feedback signal from the galvo's.
*/
bool Laser::testGalvoFeedback() {
  sendtoRaw(4000, 4000);
  delay(10);  // Delay to give the galvo's time to reach the position
  const short xPosFeedback = analogRead(_xGalvoFeedbackSignal);
  const short yPosFeedback = analogRead(_yGalvoFeedbackSignal);

  sendtoRaw(-4000, -4000);
  delay(10);  // Delay to give the galvo's time to reach the position
  const bool xGalvoFeedbackWorking = abs(xPosFeedback - analogRead(_xGalvoFeedbackSignal)) > 800;
  const bool yGalvoFeedbackWorking = abs(yPosFeedback - analogRead(_yGalvoFeedbackSignal)) > 800;

  return xGalvoFeedbackWorking && yGalvoFeedbackWorking;
}

/**
 @brief This function turns the lasers on, the laser will not output power because the watchdog is currently in the HardwareCheck state.
 @brief The watchdog verifies that the laser feedback is working and will change the state to ready if the lasers are turned off and the rest of the hardware check succeeded. 
*/
void Laser::testLaserFeedbackForWatchdog() {
  setLaserPower(10, 10, 10);
  delay(10);
  setLaserPower(0, 0, 0);
}

/**
 * Performs a hardware check for the galvo's and the laser readout
 * 
 * Returns true if the function succeeds, false if it fails
 */
bool Laser::hardwareSelfCheck() {
  const bool galvosWorking = testGalvoFeedback();
  testLaserFeedbackForWatchdog();
  const bool watchdogReady = analogRead(_safeOperationPin) > 800;

  return galvosWorking && watchdogReady;
}