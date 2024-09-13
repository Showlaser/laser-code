#ifndef MOVINGLINESANIMATION_H
#define MOVINGLINESANIMATION_H

#include "Arduino.h"
#include "IStandaloneAnimation.h"
#include "Laser.h"

class MovingLinesAnimation : public IStandaloneAnimation {
public:
  MovingLinesAnimation(Laser &laser);
  String getAnimationName();
  void execute(unsigned long animationStartedAtMillis, int timePerAnimationInMilliSeconds);

private:
  Laser& _laser;
};

#endif