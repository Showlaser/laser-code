#include "MovingLinesAnimation.h"

MovingLinesAnimation::MovingLinesAnimation(Laser &laser)
  : _laser(laser) {
}

void MovingLinesAnimation::execute(unsigned long animationStartedAtMillis, int timePerAnimationInMilliSeconds) {
  settingsModel settings = Settings::getSettings();

  unsigned int currentTime = millis();
  int y = map(currentTime, animationStartedAtMillis, animationStartedAtMillis + timePerAnimationInMilliSeconds, -4000, 4000);

  bool flipY = false;
  for (int i = -4000; i < 4000; i += 1000) {
    int r = 0;
    int g = 0;
    int b = 0;
    if (i < -1000) {
      r = settings.maxPowerPerlaserInPercentage;
    } else if (i == -1000) {
      r = 0;
      g = settings.maxPowerPerlaserInPercentage;
    } else if (i == 1000) {
      r = 0;
      g = settings.maxPowerPerlaserInPercentage;
      b = settings.maxPowerPerlaserInPercentage;
    }

    _laser.sendTo(i, flipY ? -y : y);
    _laser.setLaserPower(r, g, b);

    delayMicroseconds(10);
    flipY = !flipY;
  }

  _laser.setLaserPower(0, 0, 0);
}

String MovingLinesAnimation::getAnimationName() {
  return MovingLinesAnimationName;
}