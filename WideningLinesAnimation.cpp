#include "WideningLinesAnimation.h"

WideningLinesAnimation::WideningLinesAnimation(Laser &laser)
  : _laser(laser) {
}

void WideningLinesAnimation::execute(unsigned long animationStartedAtMillis, int timePerAnimationInMilliSeconds) {
  settingsModel settings = Settings::getSettings();

  unsigned int currentTime = millis();
  int x = map(currentTime, animationStartedAtMillis, animationStartedAtMillis + timePerAnimationInMilliSeconds, random(-4000, 4000), random(-4000, 4000));

  byte rgb[3] = {
    random(settings.maxPowerPerlaserInPercentage / 1.5, settings.maxPowerPerlaserInPercentage),
    random(settings.maxPowerPerlaserInPercentage / 1.5, settings.maxPowerPerlaserInPercentage),
    random(settings.maxPowerPerlaserInPercentage / 1.5, settings.maxPowerPerlaserInPercentage)
  };

  int y = random(-4000, 4000);

  for (int i = 0; i < 40; i++) {
    _laser.sendTo(x, y);
    _laser.setLaserPower(rgb[0], rgb[1], rgb[2]);

    delayMicroseconds(10);
    _laser.sendTo(-x, y);
    delayMicroseconds(10);
  }

  _laser.setLaserPower(0, 0, 0);
}

String WideningLinesAnimation::getAnimationName() {
  return WideningLinesAnimationName;
}