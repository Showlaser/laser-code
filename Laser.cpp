#include "Laser.h"
#include <MCP48xx.h>

// Define the MCP4822 instances and their respective CS pins
MCP4822 dac1(9);  // CS pin for DAC1
MCP4822 dac2(10); // CS pin for DAC2
MCP4822 dac3(12); // CS pin for DAC3

void Laser::configureDacs()
{
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
}

void Laser::init()
{
  configureDacs();
}

int Laser::mapPositionToResolution(int value)
{
  return map(value, -4000, 4000, 0, 4096);
}

/**
   @brief set input parameters within valid limits, creates a linear voltage from the previous to the new point (zigzag) and sends it to the laser.
   The zigzag is created to prevent the galvos from moving very abruptly to the next position which will cause the power supply to go into protection mode 

   @param xPos the position for the x axis galvo to move to
   @param yPos the position for the y axis galvo to move to
*/
void Laser::sendTo(int xPos, int yPos)
{
  xPos = fixBoundary(xPos, -4000, 4000);
  yPos = fixBoundary(yPos, -4000, 4000);

  int differenceX = xPos - _xPos;
  int differenceY = yPos - _yPos;
  int divideBy = asb(differenceX) > asb(differenceY) ? abs(differenceX / 8) : abs(differenceY / 8); # Equal time for each galvo to reach the given point
  
  int differenceXDivided = 0;
  if (xPos != _xPos) {
    differenceXDivided = differenceX / divideBy;
  }
  
  int differenceYDivided = 0;
  if (yPos != _yPos) {
    differenceYDivided = differenceY / divideBy;
  }

  int x = 0;
  int y = 0;

  for (int i = 0; i < divideBy; i++) {
    x += (int)differenceXDivided;
    y += (int)differenceYDivided;

    dac3.setVoltageA(mapPositionToResolution(x));
    dac3.setVoltageB(mapPositionToResolution(y));
    dac3.updateDAC();
  }

  _xPos = xPos;
  _yPos = yPos;

  dac3.setVoltageA(mapPositionToResolution(xPos));
  dac3.setVoltageB(mapPositionToResolution(yPos));
  dac3.updateDAC();
}

/**
   @brief sets the input value to the max or min value if it goes under or over the min and max values.

   @param input the input variable to fix
   @param min the minimum allowed value
   @param max the maximum allowed value

   @return int the value between or equal to the min or max value
*/
int Laser::fixBoundary(int input, int min, int max)
{
  if (input < min)
  {
    return min;
  }
  if (input > max)
  {
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
void Laser::setLaserPower(byte red, byte green, byte blue)
{
  const byte r = fixBoundary(red, 0, 255);
  const byte g = fixBoundary(green, 0, 255);
  const byte b = fixBoundary(blue, 0, 255);

  analogWrite(_redLaserPin, r);
  analogWrite(_greenLaserPin, g);
  analogWrite(_blueLaserPin, b);
}

/**
 @brief This function checks if the feedback of the galvo's is working by sending them to their maximums positions and reading the feedback signal from the galvo's.

 @return boolean true if the test succeeded false if the test fails
*/
bool Laser::testGalvoFeedback()
{
  sendtoRaw(0, 0);
  delay(100); // Delay to give the galvo's time to reach the position

  sendtoRaw(4000, 4000);
  delay(100); // Delay to give the galvo's time to reach the position
  const short xPosFeedback = analogRead(_xGalvoFeedbackSignal);
  const short yPosFeedback = analogRead(_yGalvoFeedbackSignal);

  sendtoRaw(-4000, -4000);
  delay(100); // Delay to give the galvo's time to reach the position
  const bool xGalvoFeedbackWorking = abs(xPosFeedback - analogRead(_xGalvoFeedbackSignal)) > 800;
  const bool yGalvoFeedbackWorking = abs(yPosFeedback - analogRead(_yGalvoFeedbackSignal)) > 800;

  return xGalvoFeedbackWorking && yGalvoFeedbackWorking;
}

/**
 @brief This function turns the lasers on, the laser will not output power because the watchdog is currently in the HardwareCheck state.
 @brief The watchdog verifies that the laser feedback is working and will change the state to ready if the lasers are turned off and the rest of the hardware check succeeded.
*/
void Laser::testLaserFeedbackForWatchdog()
{
  setLaserPower(10, 10, 10);
  delay(10);
  setLaserPower(0, 0, 0);
}

/**
 * @brief Performs a hardware check for the galvo's and the laser readout
 *
 * @return true if the function succeeds, false if it fails
 */
bool Laser::hardwareSelfCheck()
{
  const bool galvosWorking = testGalvoFeedback();
  testLaserFeedbackForWatchdog();
  const bool watchdogReady = analogRead(_safeOperationPin) > 800;

  return galvosWorking && watchdogReady;
}