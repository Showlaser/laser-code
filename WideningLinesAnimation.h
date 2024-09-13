#ifndef WIDENINGLINESANIMATION_H
#define WIDENINGLINESANIMATION_H

#include "Arduino.h"
#include "IStandaloneAnimation.h"
#include "Laser.h"

class WideningLinesAnimation : public IStandaloneAnimation {
public:
  WideningLinesAnimation(Laser &laser);
  String getAnimationName();
  void execute(unsigned long animationStartedAtMillis, int timePerAnimationInMilliSeconds);

private:
  Laser& _laser;
};

#endif