#include "RealtimePlayer.h"

RealtimePlayer *RealtimePlayer::_instance = nullptr;

void RealtimePlayer::begin(Laser &laser)
{
  _laser = &laser;
  _instance = this;
}

void RealtimePlayer::start(uint32_t baseClockHz)
{
  if (baseClockHz == 0)
  {
    return;
  }

  if (_running)
  {
    stop();
  }

  _ring.clear();
  _dwellLeft = 0;
  _underruns = 0;
  _instance = this;

  float periodMicros = 1000000.0f / (float)baseClockHz;
  _running = true;
  _timer.begin(isrTrampoline, periodMicros);
}

void RealtimePlayer::stop()
{
  _timer.end();
  _running = false;

  // Safety: always leave the laser blanked when the clock is not running.
  if (_laser != nullptr)
  {
    _laser->applyLaserPower(0, 0, 0);
  }
}

void RealtimePlayer::isrTrampoline()
{
  if (_instance != nullptr)
  {
    _instance->onTick();
  }
}

void RealtimePlayer::onTick()
{
  // Holding the current point for its dwell: do nothing this tick.
  if (_dwellLeft > 0)
  {
    _dwellLeft--;
    return;
  }

  OutPoint p;
  if (!_ring.pop(p))
  {
    // Underrun: producer fell behind. Blank rather than freeze on a stale point.
    _underruns++;
    if (_laser != nullptr)
    {
      _laser->applyLaserPower(0, 0, 0);
    }
    return;
  }

  // dwell counts total ticks this point occupies; we are spending one now.
  _dwellLeft = (p.dwell > 0) ? (uint16_t)(p.dwell - 1) : 0;

  if (_laser != nullptr)
  {
    _laser->writeGalvoRaw(p.x, p.y);
    _laser->applyLaserPower(p.r, p.g, p.b);
  }
}
