#include "RandomDotsAnimation.h"

RandomDotsAnimation::RandomDotsAnimation(Laser &laser)
  : _laser(laser) {
}

void RandomDotsAnimation::execute(unsigned long animationStartedAtMillis, int timePerAnimationInMilliSeconds) {
  settingsModel settings = Settings::getSettings();

  for (int i = 0; i < 50; i++) {
    int x = random(-4000, 4000);
    int y = random(-4000, 4000);

    _laser.sendTo(x, y);
    _laser.setLaserPower(random(5, settings.maxPowerPerlaserInPercentage),
                         random(5, settings.maxPowerPerlaserInPercentage),
                         random(5, settings.maxPowerPerlaserInPercentage));
    delayMicroseconds(300);
    _laser.setLaserPower(0, 0, 0);
    delayMicroseconds(100);
  }
}

String RandomDotsAnimation::getAnimationName() {
  return RandomDotsAnimationName;
}