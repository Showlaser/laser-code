#ifndef PLAYSDFileMODE_H
#define PLAYSDFileMODE_H

#include "Arduino.h"
#include "IMode.h"
#include "../Laser.h"

class PlaySDFileMode : public IMode {
public:
PlaySDFileMode(Laser &laser);
   void execute();
  void stop();
  virtual LaserMode getModeName();

private:
  Laser& _laser;
  unsigned long _firstExecutionStartedAtMillis;
  unsigned long _animationStartedAtMillis;
  unsigned long _animationsTotalDuration;
};

#endif