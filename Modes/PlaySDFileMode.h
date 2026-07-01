#ifndef PLAYSDFileMODE_H
#define PLAYSDFileMODE_H

#include "Arduino.h"
#include <vector>
#include "IMode.h"
#include "../Laser.h"
#include "../RealtimePlayer.h"
#include "../LasershowFile.h"

/**
  @brief Plays a lasershow from the SD card.

  This is the PRODUCER. It streams the ".lzs" show one frame at a time from the
  SD card (via LasershowFile), expands each frame into fully-resolved OutPoints,
  and feeds them into the RealtimePlayer's ring buffer. The RealtimePlayer's ISR
  clocks them out to the DACs at a fixed rate. All heavy work (interpolation,
  projection mapping, timing) happens here, off the realtime path, and only one
  frame is ever held in RAM.
*/
class PlaySDFileMode : public IMode
{
public:
  PlaySDFileMode(Laser &laser, RealtimePlayer &player);
  void execute();
  void stop();
  virtual LaserMode getModeName();

private:
  Laser &_laser;
  RealtimePlayer &_player;
  LasershowFile _show;

  // Conservative bench rate. Galvo max scan rate is only reached at small
  // angles; we start low and measure before pushing toward kpps (40000).
  uint32_t _baseClockHz = 20000;

  bool _started = false;
  bool _producingDone = false;
  long _ticksLeftInCluster = 0; // remaining point-clock ticks for current frame
  size_t _lapIndex = 0;         // round-robin position within _lap
  int _lastX = 0;               // logical galvo position carried between laps
  int _lastY = 0;

  unsigned long _lastUnderrunReport = 0; // throttles the serial diagnostic
  uint32_t _lastUnderrunCount = 0;

  std::vector<OutPoint> _lap; // one pass of the current frame, reused each frame

  void resetState();
  void buildLapFromCurrentFrame();
  void appendSegment(int x0, int y0, int x1, int y1, byte r, byte g, byte b);
};

#endif
