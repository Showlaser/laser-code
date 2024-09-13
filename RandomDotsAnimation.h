#ifndef RANDOMDOTSANIMATION_H
#define RANDOMDOTSANIMATION_H

#include "Arduino.h"
#include "IStandaloneAnimation.h"
#include "Laser.h"

class RandomDotsAnimation : public IStandaloneAnimation {
public:
  RandomDotsAnimation(Laser &laser);
  String getAnimationName();
  void execute(unsigned long animationStartedAtMillis, int timePerAnimationInMilliSeconds);

private:
  Laser& _laser;
};

#endif