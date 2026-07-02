#ifndef NETWORKPLAYMODE_H
#define NETWORKPLAYMODE_H

#include "Arduino.h"
#include "ShowPlayerMode.h"
#include "../MemoryShow.h"

/**
  @brief Plays a pattern or animation streamed live from the frontend over the
         API.

  The whole (small) show is uploaded as an ".lzs" binary blob to POST
  /live-binary, held in RAM (MemoryShow), and looped. A newly uploaded blob
  replaces the current one and restarts playback at a frame boundary. Patterns
  and animations are NOT stored on the laser (only lasershows are) -- this is the
  live-preview path.

  The uploaded blob is handed over via the LiveShowData / LiveShowPending globals
  (same handoff pattern as SelectedSDCardFilename). The receiver and this mode
  both run in the main loop, so there is no concurrency with each other; the ISR
  only ever touches the ring.
*/
class NetworkPlayMode : public ShowPlayerMode
{
public:
  NetworkPlayMode(Laser &laser, RealtimePlayer &player);
  LaserMode getModeName() override;

protected:
  ShowSource *acquireSource() override;
  void releaseSource() override;
  void onProducingFinished() override;
  void preExecute() override;

private:
  MemoryShow _mem;
};

#endif
