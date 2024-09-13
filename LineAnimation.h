#ifndef LINEANIMATION_H
#define LINEANIMATION_H

#include "Arduino.h"
#include "IStandaloneAnimation.h"
#include "Laser.h"

class LineAnimation : public IStandaloneAnimation {
public:
  LineAnimation(Laser &laser);
  String getAnimationName();
  void execute(unsigned long animationStartedAtMillis, int timePerAnimationInMilliSeconds);

private:
  Laser& _laser;
};

#endif