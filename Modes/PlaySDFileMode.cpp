#include "PlaySDFileMode.h"

// Maximum galvo step (in logical -4000..4000 units) between two emitted points.
// Smaller = smoother lines but more points per lap. This is the producer-side
// stand-in for the PC "maxStepPerTick" resample that arrives in a later phase.
static const int MAX_STEP = 80;

// How much headroom to keep before topping up, so each execute() pushes a batch
// rather than one point at a time.
static const uint16_t REFILL_BATCH = 64;

PlaySDFileMode::PlaySDFileMode(Laser &laser, RealtimePlayer &player)
    : _laser(laser), _player(player)
{
}

LaserMode PlaySDFileMode::getModeName()
{
  return LaserMode::SDCardMode;
}

void PlaySDFileMode::resetState()
{
  _started = false;
  _producingDone = false;
  _ticksLeftInCluster = 0;
  _lapIndex = 0;
  _lastX = 0;
  _lastY = 0;
  _lastUnderrunReport = 0;
  _lastUnderrunCount = 0;
  _lap.clear();
}

/**
  @brief Appends the points of one segment (excluding its start) to the current
         lap, interpolating so no step exceeds MAX_STEP, projection-mapping each
         emitted point. r/g/b = 0 means a blanked (laser-off) transit move.
*/
void PlaySDFileMode::appendSegment(int x0, int y0, int x1, int y1, byte r, byte g, byte b)
{
  int dx = x1 - x0;
  int dy = y1 - y0;
  int dist = max(abs(dx), abs(dy));
  int steps = dist / MAX_STEP;
  if (steps < 1)
  {
    steps = 1;
  }

  for (int i = 1; i <= steps; i++)
  {
    int x = x0 + (int)((long)dx * i / steps);
    int y = y0 + (int)((long)dy * i / steps);
    _laser.projectionMap(x, y);

    OutPoint p;
    p.x = (int16_t)x;
    p.y = (int16_t)y;
    p.r = r;
    p.g = g;
    p.b = b;
    p.dwell = 1;
    _lap.push_back(p);
  }
}

/**
  @brief Builds one full pass of the current frame into _lap: blanked move to
         each path's start, then lit segments between the path's points. _lap is
         replayed (repeated) to fill the frame's time budget for flicker-free
         brightness. Sourced from the frame LasershowFile just streamed in.
*/
void PlaySDFileMode::buildLapFromCurrentFrame()
{
  _lap.clear();

  const std::vector<LasershowPoint> &points = _show.points();
  const std::vector<uint16_t> &pathLengths = _show.pathLengths();

  int curX = _lastX;
  int curY = _lastY;
  size_t idx = 0;

  for (size_t pathIdx = 0; pathIdx < pathLengths.size(); pathIdx++)
  {
    uint16_t length = pathLengths[pathIdx];
    bool first = true;
    int prevX = curX;
    int prevY = curY;
    byte prevR = 0, prevG = 0, prevB = 0;

    for (uint16_t j = 0; j < length; j++)
    {
      const LasershowPoint &pt = points[idx++];

      if (first)
      {
        // Move to the path's first point with the laser blanked.
        appendSegment(curX, curY, pt.x, pt.y, 0, 0, 0);
        first = false;
      }
      else
      {
        // Lit edge from the previous point to this one (drawn in prev colour).
        appendSegment(prevX, prevY, pt.x, pt.y, prevR, prevG, prevB);
      }

      prevX = pt.x;
      prevY = pt.y;
      prevR = pt.r;
      prevG = pt.g;
      prevB = pt.b;
      curX = pt.x;
      curY = pt.y;
    }
  }

  _lastX = curX;
  _lastY = curY;
}

void PlaySDFileMode::execute()
{
  if (SelectedSDCardFilename == "")
  {
    return;
  }

  if (!_started)
  {
    resetState();
    if (!_show.open(SelectedSDCardFilename))
    {
      Serial.println("Failed to open show file: " + SelectedSDCardFilename);
      CurrentLaserMode = LaserMode::NotSelected;
      return;
    }
    _started = true;
    _player.start(_baseClockHz);
  }

  // Producer: keep the ring topped up. Each frame's points are emitted at
  // dwell=1 and repeated to exactly fill that frame's tick budget, so the show
  // plays at correct wall-clock speed regardless of the base clock.
  while (_player.freeSpace() > REFILL_BATCH)
  {
    if (_ticksLeftInCluster <= 0)
    {
      if (!_show.readNextFrame())
      {
        if (!LoopSDCardPlayback)
        {
          _producingDone = true;
          break;
        }
        _show.rewind();
        if (!_show.readNextFrame())
        {
          _producingDone = true; // empty show; nothing to loop
          break;
        }
      }

      buildLapFromCurrentFrame();
      long ms = _show.currentDurationMs();
      _ticksLeftInCluster = ms * (long)_baseClockHz / 1000L;
      _lapIndex = 0;

      if (_lap.empty())
      {
        _ticksLeftInCluster = 0; // nothing to draw this frame; advance
        continue;
      }
    }

    _player.push(_lap[_lapIndex]);
    _lapIndex = (_lapIndex + 1) % _lap.size();
    _ticksLeftInCluster--;
  }

  // Diagnostic: report ring underruns at most once a second, and only when the
  // count changed, so sustained producer starvation is visible without spam.
  unsigned long now = millis();
  if (now - _lastUnderrunReport >= 1000)
  {
    uint32_t underruns = _player.underruns();
    if (underruns != _lastUnderrunCount)
    {
      Serial.print("Ring underruns: ");
      Serial.println(underruns);
      _lastUnderrunCount = underruns;
    }
    _lastUnderrunReport = now;
  }

  // Finished once everything is produced AND the ring has drained.
  if (_producingDone && _player.queued() == 0)
  {
    _player.stop();
    _laser.setLaserPower(0, 0, 0);
    _show.close();
    CurrentLaserMode = LaserMode::NotSelected;
    resetState();
  }
}

void PlaySDFileMode::stop()
{
  _player.stop();
  _laser.setLaserPower(0, 0, 0);
  _show.close();
  resetState();
}
