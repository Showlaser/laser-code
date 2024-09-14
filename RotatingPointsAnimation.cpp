#include "RotatingPointsAnimation.h"

// Calculate the current radius based on the progress (from 0 to maxRadius)
const int _points = 5;  // Number of _points to draw the circle
int _pointsColors[_points][3];
const float theta_increment = 2 * PI / _points;
unsigned long previousAnimationStartedAtMillis;

const int distanceBetweenCenter = 4000;
const int cX = 0;
const int cY = 0;
float theta_rotate = 2 * PI / _points;  // Rotation angle (in radians), change to rotate by different angles

RotatingPointsAnimation::RotatingPointsAnimation(Laser &laser)
  : _laser(laser) {
}

void RotatingPointsAnimation::execute(unsigned long animationStartedAtMillis, int timePerAnimationInMilliSeconds) {
  settingsModel settings = Settings::getSettings();

  if (_pointsColors[0][0] == 0 || previousAnimationStartedAtMillis != animationStartedAtMillis) {
    previousAnimationStartedAtMillis = animationStartedAtMillis;

    for (int i = 0; i < _points; i++) {
      for (int j = 0; j < 3; j++) {
        bool on = random(0, 100) > 50;
        _pointsColors[i][j] = on ? settings.maxPowerPerlaserInPercentage : 0;
      }
    }
  }

  float rotation_speed = 0.04 / (timePerAnimationInMilliSeconds / 1000);  // Speed of rotation (adjust to control how fast the points rotate)

  for (int i = 0; i < _points; i++) {
    // Calculate the angle for each point (from 0 to 2 * PI)
    float theta = i * theta_increment;

    // Calculate the x and y coordinates using the parametric circle equations
    float x = cX + distanceBetweenCenter * cos(theta);
    float y = cY + distanceBetweenCenter * sin(theta);

    // rotate points around center point
    x = cX + (x - cX) * cos(theta_rotate) - (y - cY) * sin(theta_rotate);
    y = y + (x - cX) * sin(theta_rotate) + (y - cY) * cos(theta_rotate);

    _laser.sendTo(x, y);
    _laser.setLaserPower(_pointsColors[i][0], _pointsColors[i][1], _pointsColors[i][2]);
    delayMicroseconds(150);

    _laser.setLaserPower(0, 0, 0);
    delayMicroseconds(150);
  }

  theta_rotate += rotation_speed;  // Increase rotation angle over time to create continuous rotation
}

String RotatingPointsAnimation::getAnimationName() {
  return RotatingPointsAnimationName;
}