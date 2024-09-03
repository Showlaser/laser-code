#ifndef BREATHINGCIRCLEANIMATION_H
#define BREATHINGCIRCLEANIMATION_H

#include "Arduino.h"
#include "IStandaloneAnimation.h"
#include "Laser.h"

class BreathingCircleAnimation : public IStandaloneAnimation {
public:
  BreathingCircleAnimation(Laser &laser);
  String getAnimationName();
  void execute(unsigned long animationStartedAtMillis, int timePerAnimationInSeconds);

private:
  Laser& _laser;
};

#endif