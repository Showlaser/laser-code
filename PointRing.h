#ifndef POINTRING_H
#define POINTRING_H

#include "Arduino.h"

/**
  @brief A single fully-expanded laser point ready for direct DAC output.

  This is what the realtime consumer pops and writes; all decoding,
  interpolation and resampling has already happened in the producer.
  r/g/b are 0..100 laser power percentages (same units as setLaserPower).
  dwell is the number of base-clock ticks this point is held before the
  next point is popped (1 = output for a single tick).
*/
struct __attribute__((packed)) OutPoint
{
  int16_t x;
  int16_t y;
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint16_t dwell;
};

/**
  @brief Lock-free single-producer / single-consumer ring buffer of OutPoints.

  The producer (non-realtime, main loop) only calls push()/freeSpace().
  The consumer (realtime IntervalTimer ISR) only calls pop().
  With exactly one producer and one consumer, and head/tail updated last
  and marked volatile, no locking is required on the Teensy.

  Capacity must be a power of two; usable capacity is N-1 (one slot is kept
  empty to distinguish full from empty).
*/
template <uint16_t N>
class PointRing
{
public:
  static_assert((N & (N - 1)) == 0, "PointRing size must be a power of two");

  /**
    @brief Consumer side: pop the next point. Trivial enough for an ISR.
    @param out filled with the next point on success
    @return true if a point was returned, false if the buffer is empty (underrun)
  */
  inline bool pop(OutPoint &out)
  {
    if (_tail == _head)
    {
      return false; // underrun
    }
    out = _buffer[_tail & _mask];
    _tail = (uint16_t)(_tail + 1);
    return true;
  }

  /**
    @brief Producer side: number of points that can still be pushed.
  */
  inline uint16_t freeSpace() const
  {
    return (uint16_t)((N - 1) - ((_head - _tail) & _mask));
  }

  /**
    @brief Producer side: true if at least one point can be pushed.
  */
  inline bool canPush() const
  {
    return ((uint16_t)(_head - _tail) & _mask) != _mask;
  }

  /**
    @brief Producer side: push a point. Caller must ensure space via canPush()/freeSpace().
    @return true if pushed, false if the buffer was full (point dropped)
  */
  inline bool push(const OutPoint &p)
  {
    if (!canPush())
    {
      return false; // full
    }
    _buffer[_head & _mask] = p;
    _head = (uint16_t)(_head + 1);
    return true;
  }

  /**
    @brief Number of points currently queued.
  */
  inline uint16_t count() const
  {
    return (uint16_t)((_head - _tail) & _mask);
  }

  /**
    @brief Reset the buffer. Only safe to call when the consumer ISR is stopped.
  */
  inline void clear()
  {
    _head = 0;
    _tail = 0;
  }

private:
  static const uint16_t _mask = N - 1;
  OutPoint _buffer[N];
  volatile uint16_t _head = 0; // producer writes, consumer reads
  volatile uint16_t _tail = 0; // consumer writes, producer reads
};

#endif
