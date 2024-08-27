#include "Laser.h"
#include <MCP48xx.h>

// Define the MCP4822 instances and their respective CS pins
MCP4822 dac1(9);   // CS pin for DAC1
MCP4822 dac2(10);  // CS pin for DAC2
MCP4822 dac3(12);  // CS pin for DAC3

void Laser::configureDacs() {
  dac1.init();
  dac2.init();
  dac3.init();

  // Turn on both channels A and B for DACs
  dac1.turnOnChannelA();
  dac1.turnOnChannelB();
  dac2.turnOnChannelA();
  dac2.turnOnChannelB();
  dac3.turnOnChannelA();
  dac3.turnOnChannelB();

  // Configure DACs' channels in High gain mode (optional, as it's the default)
  dac1.setGainA(MCP4822::Low);
  dac1.setGainB(MCP4822::Low);
  dac2.setGainA(MCP4822::Low);
  dac2.setGainB(MCP4822::Low);
  dac3.setGainA(MCP4822::Low);
  dac3.setGainB(MCP4822::Low);

  dac1.setVoltageA(0);
  dac1.setVoltageB(0);
  dac1.updateDAC();

  dac2.setVoltageA(0);
  dac2.setVoltageB(0);
  dac2.updateDAC();

  dac3.setVoltageA(0);
  dac3.setVoltageB(0);
  dac3.updateDAC();
}

void Laser::init(WDT_T4<WDT1> &watchdog, settingsModel laserSettings) {
  _watchdog = watchdog;
  _laserSettings = laserSettings;
  configureDacs();
}

/**
   @brief set input parameters within valid limits, creates a linear voltage from the previous to the new point (zigzag) and sends it to the laser.
   The zigzag is created to prevent the galvos from moving very abruptly to the next position which will cause the power supply to go into protection mode

   @param xPos the position for the x axis galvo to move to
   @param yPos the position for the y axis galvo to move to
*/

/**
  @brief sends the galvos to the specified position using logistic growth 
*/
void Laser::sendTo(int newXPos, int newYPos) {
  _watchdog.feed();

  int xPos = fixBoundary(newXPos, -4000, 4000);
  int yPos = fixBoundary(newYPos, -4000, 4000);

  int durationXGalvo = abs(xPos - _xPos) / 2;  // Duration of the entire cycle in microseconds
  int durationYGalvo = abs(yPos - _yPos) / 2;  // Duration of the entire cycle in microseconds
  int duration = (durationXGalvo >= durationYGalvo ? durationXGalvo : durationYGalvo);

  float middlePoint = duration / 2.0;
  float growRate = 0.008;

  unsigned long startTime = micros();
  unsigned long endTime = startTime + duration;

  // positive or negative logistic growth curves this helps to smooth the galvo movement
  while (micros() < endTime) {
    float elapsedTime = micros() - startTime;

    if (_xPos < xPos) {
      int xMappedToVoltage = xPos = map(xPos, -4000, 4000, 0, 4096);
      float xVoltage = xMappedToVoltage / (1.0 + exp(-growRate * (elapsedTime - middlePoint)));
      dac3.setVoltageA(xVoltage);
    } else {
      int xMappedToVoltage = xPos = map(xPos, -4000, 4000, 0, 4096);
      float xVoltage = xMappedToVoltage / (1.0 + exp(-growRate * ((duration - elapsedTime) - middlePoint)));
      dac3.setVoltageA(xVoltage);
    }

    if (_yPos < yPos) {
      int yMappedToVoltage = yPos = map(xPos, -4000, 4000, 0, 4096);
      float yVoltage = yMappedToVoltage / (1.0 + exp(-growRate * (elapsedTime - middlePoint)));
      dac3.setVoltageB(yVoltage);
    } else {
      int yMappedToVoltage = yPos = map(xPos, -4000, 4000, 0, 4096);
      float yVoltage = yMappedToVoltage / (1.0 + exp(-growRate * ((duration - elapsedTime) - middlePoint)));
      dac3.setVoltageB(yVoltage);
    }

    dac3.updateDAC();
  }

  _xPos = xPos;
  _yPos = yPos;

  _watchdog.feed();
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
    return min;
  }
  if (input > max) {
    return max;
  }
  return input;
}

/**
 @brief This function sets the power of the laser by the provided values.
 @brief Values below 0 will be set to 0 and values above 255 will be set to 255

 @param red the power the red laser should output from 0 / 255
 @param green the power the green laser should output from 0 / 255
 @param blue the power the blue laser should output from 0 / 255
*/
void Laser::setLaserPower(byte red, byte green, byte blue) {
  if (_laserOutputDisabled) {
    return;
  }

  byte r = fixBoundary(red, 0, 255);
  byte g = fixBoundary(green, 0, 255);
  byte b = fixBoundary(blue, 0, 255);

  int currentMaxPowerRgb = r + g + b;
  if (currentMaxPowerRgb > _laserSettings.maxPowerRgb) {
    // limit the laser power
    r = _laserSettings.maxPowerRgb * (r / 765);
    g = _laserSettings.maxPowerRgb * (r / 765);
    b = _laserSettings.maxPowerRgb * (r / 765);
  }

  dac1.setVoltageA(map(r, 0, 255, 0, 4096));
  dac1.setVoltageB(map(g, 0, 255, 0, 4096));
  dac1.updateDAC();

  dac2.setVoltageA(map(b, 0, 255, 0, 4096));
  dac2.updateDAC();
}

/**
  @brief This function turns off the lasers and disables the possibility to turn them on
         this can be used in case there is an emergency. Call enableLasers to enable the laser module again
*/
void Laser::disableLasers() {
  _laserOutputDisabled = true;
}

/**
  @brief This function enables the laser module to be turned on again 
*/
void Laser::enableLasers() {
  _laserOutputDisabled = false;
}

/**
 @brief This function checks if the feedback of the galvo's is working by sending them to their maximums positions and reading the feedback signal from the galvo's.
 @return boolean true if the test succeeded false if the test fails
*/
bool Laser::testGalvoFeedback() {
  int previousXFeedback = -9999;
  int previousYFeedback = -9999;

  for (int i = -4000; i < 4000; i += 100) {
    sendTo(-4000, -4000);
    int xGalvoFeedbackReading = analogRead(_xGalvoFeedbackSignal);
    int yGalvoFeedbackReading = analogRead(_yGalvoFeedbackSignal);

    if (xGalvoFeedbackReading == previousXFeedback || yGalvoFeedbackReading == previousYFeedback) {
      return false;
    }
  }

  return true;
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
 * @brief Performs a hardware check for the galvo's and the laser readout
 *
 * @return true if the function succeeds, false if it fails
 */
bool Laser::hardwareSelfCheck() {
  const bool galvosWorking = testGalvoFeedback();
  testLaserFeedbackForWatchdog();
  const bool watchdogReady = analogRead(_safeOperationPin) > 800;

  return galvosWorking && watchdogReady;
}