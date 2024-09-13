#include "CircleAnimation.h"

// Define the circle's radius
const int radius = 400;  // Adjust the radius as needed

// Define the number of steps around the circle (more steps = smoother circle)
const int steps = 360;  // 360 steps for full circle

CircleAnimation::CircleAnimation(Laser &laser)
  : _laser(laser) {
}

void CircleAnimation::execute(unsigned long animationStartedAtMillis, int timePerAnimationInMilliSeconds) {
  settingsModel settings = Settings::getSettings();
  const float maxRadius = 4000;  // Maximum radius the circle can reach

  unsigned long currentMillis = millis();

  // Calculate the elapsed time
  unsigned long elapsedTime = currentMillis - animationStartedAtMillis;

  // Calculate the progress as a fraction (from 0.0 to 1.0)
  float progress = (float)(elapsedTime % timePerAnimationInMilliSeconds) / timePerAnimationInMilliSeconds;

  // Calculate the current radius based on the progress (from 0 to maxRadius)
  float radius = progress * maxRadius;
  const int points = 100;  // Number of points to draw the circle

  for (int i = 0; i < points; i++) {
    // Calculate the angle for each point (from 0 to 2 * PI)
    float theta = 2 * PI * i / points;

    // Calculate the x and y coordinates using the parametric circle equations
    float x = radius * cos(theta);
    float y = radius * sin(theta);

    _laser.sendTo(x, y);
    _laser.setLaserPower(random(0, settings.maxPowerPerlaserInPercentage),
                         random(0, settings.maxPowerPerlaserInPercentage),
                         random(0, settings.maxPowerPerlaserInPercentage));
    delayMicroseconds(100);
    _laser.setLaserPower(0, 0, 0);
    delayMicroseconds(100);
  }
}

String CircleAnimation::getAnimationName() {
  return CircleAnimationName;
}