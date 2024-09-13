#include "StandaloneMode.h"
#include "IStandaloneAnimation.h"
#include "RandomDotsAnimation.h"
#include "LineAnimation.h"
#include "CircleAnimation.h"
#include "MovingLinesAnimation.h"
#include "WideningLinesAnimation.h"

const int _animationsLength = 5;
IStandaloneAnimation* _animations[_animationsLength];

int _previousAnimationId = 0;
unsigned long _firstExecutionStartedAtMillis = 0;
unsigned long _animationStartedAtMillis = 0;
const unsigned int _timePerAnimationInMilliSeconds = 4000;

unsigned long _animationsTotalDuration = _firstExecutionStartedAtMillis + (_timePerAnimationInMilliSeconds * _animationsLength);

String _nameOfCurrentAnimationToDisplay;

StandaloneMode::StandaloneMode(Laser& laser)
  : _laser(laser) {
  _animations[0] = new RandomDotsAnimation(_laser);
  _animations[1] = new LineAnimation(_laser);
  _animations[2] = new CircleAnimation(_laser);
  _animations[3] = new MovingLinesAnimation(_laser);
  _animations[4] = new WideningLinesAnimation(_laser);
}

int StandaloneMode::getSelectedAnimationId() {
  return (int)map(millis(),
                  _firstExecutionStartedAtMillis,
                  _animationsTotalDuration,
                  0,
                  _animationsLength - 1);
}

void StandaloneMode::execute() {
  if (_firstExecutionStartedAtMillis == 0 || _animationsTotalDuration < millis()) {
    _firstExecutionStartedAtMillis = millis();
    _animationsTotalDuration = millis() + (_timePerAnimationInMilliSeconds * _animationsLength);
  }

  int animationId = getSelectedAnimationId();
  if (animationId != _previousAnimationId) {
    _previousAnimationId = animationId;
    _animationStartedAtMillis = millis();
  }
  
  _animations[animationId]->execute(_animationStartedAtMillis, _timePerAnimationInMilliSeconds);
}

void StandaloneMode::stop() {
  _firstExecutionStartedAtMillis = 0;
  _animationsTotalDuration = millis() + (_timePerAnimationInMilliSeconds * _animationsLength);
  _laser.setLaserPower(0, 0, 0);
}

LaserMode StandaloneMode::getModeName() {
  return LaserMode::Standalone;
}