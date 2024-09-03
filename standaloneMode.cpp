#include "StandaloneMode.h"
#include "IStandaloneAnimation.h"
#include "BreathingCircleAnimation.h"

const int _animationsLength = 1;
IStandaloneAnimation* _animations[_animationsLength];

unsigned long _firstExecutionStartedAtMillis = 0;
unsigned long _animationStartedAtMillis = 0;
int _timePerAnimationInSeconds = 3000;

unsigned long _animationsTotalDuration = _firstExecutionStartedAtMillis + (_timePerAnimationInSeconds * _animationsLength);

String _nameOfCurrentAnimationToDisplay;

StandaloneMode::StandaloneMode(Laser& laser)
  : _laser(laser) {
  _animations[0] = new BreathingCircleAnimation(_laser);
}

int StandaloneMode::getSelectedAnimationId() {
  return (int)map(millis(),
                  0,
                  _animationsTotalDuration,
                  0,
                  _animationsLength);
}

void StandaloneMode::execute() {
  if (_firstExecutionStartedAtMillis == 0 || _animationsTotalDuration > millis()) {
    _firstExecutionStartedAtMillis = millis();
  }

  int animationId = getSelectedAnimationId();
  _animations[animationId]->execute(_animationStartedAtMillis, _timePerAnimationInSeconds);
}

void StandaloneMode::stop() {
  _firstExecutionStartedAtMillis = 0;
}

LaserMode StandaloneMode::getModeName() {
  return LaserMode::Standalone;
}