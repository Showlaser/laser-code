#ifndef PLAYSDFileMODE_H
#define PLAYSDFileMODE_H

#include "Arduino.h"
#include "IMode.h"
#include "../Laser.h"

class PlaySDFileMode : public IMode
{
public:
  PlaySDFileMode(Laser &laser);
  void execute();
  void stop();
  virtual LaserMode getModeName();

private:
  Laser &_laser;
  unsigned long roundDown(unsigned long numToRound);
  JsonObject getCurrentClusterToPlay(JsonArray laserCommands, bool &success);
  void playCluster(JsonObject cluster);
  unsigned long _firstExecutionStartedAtMillis = 4294967295; // set to max value
  String lasershowJson;
};

#endif