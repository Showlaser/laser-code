#include "BreathingCircleAnimation.h"

BreathingCircleAnimation::BreathingCircleAnimation(Laser &laser) : _laser(laser) {

}

void BreathingCircleAnimation::execute(unsigned long animationStartedAtMillis, int timePerAnimationInSeconds) {

}

String BreathingCircleAnimation::getAnimationName() {
  return BreathingCircleAnimationName;
}