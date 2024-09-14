#ifndef ROTATINGPOINTSANIMATION_H
#define ROTATINGPOINTSANIMATION_H

#include "Arduino.h"
#include "IStandaloneAnimation.h"
#include "Laser.h"

class RotatingPointsAnimation : public IStandaloneAnimation {
public:
  RotatingPointsAnimation(Laser& laser);
  String getAnimationName();
  void execute(unsigned long animationStartedAtMillis, int timePerAnimationInMilliSeconds);

private:
  float theta_rotate;
  Laser& _laser;
};

#endif