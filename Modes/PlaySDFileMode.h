#ifndef PLAYSDFileMODE_H
#define PLAYSDFileMODE_H

#include "Arduino.h"
#include "ShowPlayerMode.h"
#include "../LasershowFile.h"

/**
  @brief Plays a lasershow from the SD card.

  Streams the ".lzs" show one frame at a time from the SD card (via
  LasershowFile); the shared ShowPlayerMode producer expands each frame and
  clocks it out through the RealtimePlayer. Plays once, then returns to no mode.
*/
class PlaySDFileMode : public ShowPlayerMode
{
public:
  PlaySDFileMode(Laser &laser, RealtimePlayer &player);
  LaserMode getModeName() override;

protected:
  ShowSource *acquireSource() override;
  void releaseSource() override;
  void onProducingFinished() override;

private:
  LasershowFile _file;
};

#endif
