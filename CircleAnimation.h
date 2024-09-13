#ifndef CIRCLEANIMATION_H
#define CIRCLEANIMATION_H

#include "Arduino.h"
#include "IStandaloneAnimation.h"
#include "Laser.h"

class CircleAnimation : public IStandaloneAnimation {
public:
  CircleAnimation(Laser &laser);
  String getAnimationName();
  void execute(unsigned long animationStartedAtMillis, int timePerAnimationInMilliSeconds);

private:
  Laser& _laser;
};

#endif