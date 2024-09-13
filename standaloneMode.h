#ifndef STANDALONEMODE_H
#define STANDALONEMODE_H

#include "Arduino.h"
#include "IMode.h"
#include "Laser.h"

class StandaloneMode : public IMode {
public:
  StandaloneMode(Laser &laser);
 
  void execute();
  void stop();
  virtual LaserMode getModeName();

private:
  Laser& _laser;
  unsigned long _firstExecutionStartedAtMillis;
  unsigned long _animationStartedAtMillis;
  unsigned int _timePerAnimationInSeconds;
  unsigned long _animationsTotalDuration;
  String _nameOfCurrentAnimationToDisplay;

  /**
  @brief calculates the animation id by time in millis, the maximum time based on the  _firstExecutionStartedAtMillis, _timePerAnimationInSeconds and _animationsLength fields
  @returns the current animation id that should be displayed
  */
  int getSelectedAnimationId();
};

#endif