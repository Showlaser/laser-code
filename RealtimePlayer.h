#ifndef REALTIMEPLAYER_H
#define REALTIMEPLAYER_H

#include "Arduino.h"
#include "PointRing.h"
#include "Laser.h"

// Usable capacity is REALTIME_RING_SIZE-1 points. 4096 * sizeof(OutPoint)=~32KB
// of RAM and ~205ms of lookahead at 20kHz / ~102ms at 40kHz.
#ifndef REALTIME_RING_SIZE
#define REALTIME_RING_SIZE 4096
#endif

/**
  @brief The realtime playback engine: a constant-rate IntervalTimer pops
         pre-expanded points from a ring buffer and writes them to the DACs.

  The PRODUCER (main loop, non-realtime) calls freeSpace()/push() to keep the
  ring full. The CONSUMER is onTick(), driven by the IntervalTimer ISR; it does
  the absolute minimum: pop -> fast galvo write -> power clamp+write, and blanks
  the laser on underrun. SD reads and decoding never happen here.
*/
class RealtimePlayer
{
public:
  /**
    @brief Binds the player to a Laser. Must be called once before start().
  */
  void begin(Laser &laser);

  /**
    @brief Starts the point clock at the given base rate and clears the ring.
    @param baseClockHz output rate in points/second (e.g. 20000 for bench)
  */
  void start(uint32_t baseClockHz);

  /**
    @brief Stops the point clock and blanks the laser. Safe to call any time.
  */
  void stop();

  bool isRunning() const { return _running; }

  // Number of ISR ticks since start() that found the ring empty (producer fell
  // behind). A few at show start/end are normal; sustained growth mid-show means
  // the producer can't keep up at the current base clock.
  uint32_t underruns() const { return _underruns; }

  // --- Producer side ---
  uint16_t freeSpace() { return _ring.freeSpace(); }
  bool push(const OutPoint &p) { return _ring.push(p); }
  uint16_t queued() { return _ring.count(); }

  // --- Consumer side (called from the ISR via the trampoline) ---
  void onTick();

private:
  static void isrTrampoline();
  static RealtimePlayer *_instance;

  Laser *_laser = nullptr;
  IntervalTimer _timer;
  PointRing<REALTIME_RING_SIZE> _ring;
  volatile uint16_t _dwellLeft = 0;
  volatile bool _running = false;
  volatile uint32_t _underruns = 0;
};

#endif
