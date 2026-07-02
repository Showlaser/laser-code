#ifndef SHOWPLAYERMODE_H
#define SHOWPLAYERMODE_H

#include "Arduino.h"
#include <vector>
#include "IMode.h"
#include "../Laser.h"
#include "../RealtimePlayer.h"
#include "../ShowSource.h"

/**
  @brief Base class for modes that PLAY an ".lzs" show (the PRODUCER).

  Streams the show one frame at a time from a ShowSource, expands each frame into
  fully-resolved OutPoints (a blanked move to each path's start, then lit
  segments between its points, interpolated so no step exceeds MAX_STEP), and
  feeds them into the RealtimePlayer's ring. The RealtimePlayer's ISR clocks them
  out to the DACs at a fixed rate. All the heavy work happens here, off the
  realtime path, and only one frame is ever held in RAM.

  Subclasses supply the concrete ShowSource and the start/finish policy:
    - acquireSource(): open/load the source; return nullptr = nothing to play yet.
    - releaseSource(): close it.
    - onProducingFinished(): what to do once a (non-looping) show has fully drained.
    - preExecute(): optional per-call hook (e.g. detect a new live blob).
*/
class ShowPlayerMode : public IMode
{
public:
  ShowPlayerMode(Laser &laser, RealtimePlayer &player);
  void execute() override;
  void stop() override;

protected:
  Laser &_laser;
  RealtimePlayer &_player;

  // Conservative bench rate. Galvo max scan rate is only reached at small
  // angles; we start low and measure before pushing toward kpps (40000).
  uint32_t _baseClockHz = 20000;
  bool _loop = false; // when true, the show replays from the start instead of ending

  // --- Subclass hooks ---
  // Returns the source to play, or nullptr if there is nothing to play right now
  // (execute() then simply idles until there is).
  virtual ShowSource *acquireSource() = 0;
  virtual void releaseSource() = 0;
  virtual void onProducingFinished() = 0;
  virtual void preExecute() {}

  // Tears playback down (stops the clock, blanks the laser, releases the source,
  // resets state) so the next execute() re-acquires the source. Subclasses use
  // this to restart playback, e.g. when a new live blob arrives.
  void restart();

private:
  ShowSource *_source = nullptr;
  bool _started = false;
  bool _producingDone = false;
  bool _needNewFrame = true;    // load the next frame's lap on the next iteration
  long _ticksLeftInCluster = 0; // remaining point-clock ticks for current frame
  long _lapTicks = 0;           // total ticks one full pass of _lap occupies (dwell-aware)
  size_t _lapIndex = 0;         // round-robin position within _lap
  int _lastX = 0;               // logical galvo position carried between laps
  int _lastY = 0;

  unsigned long _lastUnderrunReport = 0; // throttles the serial diagnostic
  uint32_t _lastUnderrunCount = 0;

  std::vector<OutPoint> _lap; // one pass of the current frame, reused each frame

  void resetState();
  void buildLapFromCurrentFrame();
  void appendSegment(int x0, int y0, int x1, int y1, byte r, byte g, byte b);
  void reportUnderruns();
};

#endif
