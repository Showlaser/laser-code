#include "LineAnimation.h"

LineAnimation::LineAnimation(Laser &laser)
  : _laser(laser) {
}

void LineAnimation::execute(unsigned long animationStartedAtMillis, int timePerAnimationInMilliSeconds) {
  settingsModel settings = Settings::getSettings();

  unsigned int currentTime = millis();
  int y = map(currentTime, animationStartedAtMillis, animationStartedAtMillis + timePerAnimationInMilliSeconds, -4000, 4000);

  byte rgb[3]= { 0, 0, 0 };
  int color = (int)map(currentTime,
                       animationStartedAtMillis,
                       animationStartedAtMillis + timePerAnimationInMilliSeconds,
                       0,
                       2);

  if (color == 0) {
    rgb[0] = settings.maxPowerPerlaserInPercentage;
  } else if (color == 1) {
    rgb[0] = 0;
    rgb[1] = settings.maxPowerPerlaserInPercentage;
  } else if (color == 2) {
    rgb[0] = 0;
    rgb[1] = 0;
    rgb[2] = settings.maxPowerPerlaserInPercentage;
  }

  _laser.sendTo(-4000, y);
  _laser.setLaserPower(rgb[0], rgb[1], rgb[2]);

  delayMicroseconds(10);
  _laser.sendTo(4000, y);
  delayMicroseconds(10);
}

String LineAnimation::getAnimationName() {
  return LineAnimationName;
}