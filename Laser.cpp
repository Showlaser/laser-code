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
  dac1.setGainA(MCP4822::High);
  dac1.setGainB(MCP4822::High);
  dac2.setGainA(MCP4822::High);
  dac2.setGainB(MCP4822::High);
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

void Laser::init(WDT_T4<WDT1> &watchdog) {
  _watchdog = watchdog;
  configureDacs();
}

/**
  @brief sends the galvos to the specified position using logistic growth 
*/
void Laser::sendTo(int newXPos, int newYPos) {
  _watchdog.feed();

  int xPos = fixBoundary(newXPos, -4000, 4000);
  int yPos = fixBoundary(newYPos, -4000, 4000);

  settingsModel settings = Settings::getSettings();
  int projectionTopInPx = map(settings.projectionTopInPercentage, 0, 100, -4000, 4000);
  int projectionBottomInPx = map(settings.projectionBottomInPercentage, 0, 100, 4000, -4000);

  int projectionLeftInPx = map(settings.projectionLeftInPercentage, 0, 100, -4000, 4000);
  int projectionRightInPx = map(settings.projectionRightInPercentage, 0, 100, 4000, -4000);

  xPos = map(xPos, -4000, 4000, projectionLeftInPx, projectionRightInPx);
  yPos = map(yPos, -4000, 4000, projectionBottomInPx, projectionTopInPx);

  int durationXGalvo = abs(xPos - _xPos) + 30;
  int durationYGalvo = abs(yPos - _yPos) + 30;
  int duration = (durationXGalvo >= durationYGalvo ? durationXGalvo : durationYGalvo) / 5;

  float middlePoint = duration / 2.0;
  float growRate = 0.0012;

  unsigned long startTime = micros();
  unsigned long endTime = startTime + duration;

  int xMappedToVoltage = map(xPos, -4000, 4000, 0, 4096);
  dac3.setVoltageA(xMappedToVoltage);

  int yMappedToVoltage = map(yPos, -4000, 4000, 0, 4096);
  dac3.setVoltageB(yMappedToVoltage);
  dac3.updateDAC();

  delayMicroseconds(duration);

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
 @brief Values below 0 will be set to 0 and values above 100 will be set to 100

 @param red the power the red laser should output from 0 / 100
 @param green the power the green laser should output from 0 / 100
 @param blue the power the blue laser should output from 0 / 100
*/
void Laser::setLaserPower(byte red, byte green, byte blue) {
  if (_laserOutputDisabled) {
    Serial.println("Output disabled!");
    return;
  }

  byte r = fixBoundary(red, 0, 100);
  byte g = fixBoundary(green, 0, 100);
  byte b = fixBoundary(blue, 0, 100);

  int currentMaxPowerRgbPercentage = r + g + b;
  settingsModel settings = Settings::getSettings();
  if (currentMaxPowerRgbPercentage > (settings.maxPowerPerlaserInPercentage * 3)) {
    // limit the laser power
    r = settings.maxPowerPerlaserInPercentage * (r / 100);
    g = settings.maxPowerPerlaserInPercentage * (r / 100);
    b = settings.maxPowerPerlaserInPercentage * (r / 100);
  }

  dac1.setVoltageA(map(r, 0, 100, 0, 3500));
  dac1.setVoltageB(map(g, 0, 100, 50, 3900));
  dac1.updateDAC();

  dac2.setVoltageA(map(b, 0, 100, 0, 2750));
  dac2.updateDAC();
}

/**
  @brief This function turns off the lasers and disables the possibility to turn them on
         this can be used in case there is an emergency. Call enableLasers to enable the laser module again
*/
void Laser::disableLasers() {
  dac1.setVoltageA(0);
  dac1.setVoltageB(0);
  dac1.updateDAC();

  dac2.setVoltageA(0);
  dac2.updateDAC();

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
  sendTo(-4000, -4000);
  delay(500);

  int previousXFeedback = analogRead(_xGalvoFeedbackSignal);
  int previousYFeedback = analogRead(_yGalvoFeedbackSignal);

  sendTo(4000, 4000);
  delay(500);

  int xGalvoFeedbackReading = analogRead(_xGalvoFeedbackSignal);
  int yGalvoFeedbackReading = analogRead(_yGalvoFeedbackSignal);

  return abs(previousXFeedback - xGalvoFeedbackReading) > 100 && abs(previousYFeedback - yGalvoFeedbackReading) > 100;
}