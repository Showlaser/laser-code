#include "ShowPlayerMode.h"

// Maximum galvo step (in logical -4000..4000 units) between two emitted points.
// Smaller = smoother lines but more points per lap. This is the producer-side
// stand-in for the PC "maxStepPerTick" resample that arrives in a later phase.
static const int MAX_STEP = 80;

// How much headroom to keep before topping up, so each execute() pushes a batch
// rather than one point at a time.
static const uint16_t REFILL_BATCH = 64;

// Time to hold the beam at each path vertex (corner). A galvo cannot turn a
// sharp corner instantly; without a brief pause it "cuts the corner" and the
// shape rounds/bows. The hold is specified in TIME and converted to base-clock
// ticks per show, so corners settle equally long at every point rate.
static const uint32_t CORNER_DWELL_US = 200;

// Allowed range for the point clock. A show's exported kpps is honored within
// this range. 40k matches the exporter's default galvo speed; the galvos only
// truly track that rate at small deflection angles -- large full-field moves
// lag and soften, which is scanner physics, not a bug. The minimum guards
// against absurdly slow clocks from a malformed file.
static const uint32_t MAX_BASE_CLOCK_HZ = 40000;
static const uint32_t MIN_BASE_CLOCK_HZ = 1000;

ShowPlayerMode::ShowPlayerMode(Laser &laser, RealtimePlayer &player)
    : _laser(laser), _player(player)
{
}

void ShowPlayerMode::resetState()
{
  _source = nullptr;
  _started = false;
  _producingDone = false;
  _needNewFrame = true;
  _ticksLeftInCluster = 0;
  _lapTicks = 0;
  _lapIndex = 0;
  _lastX = 0;
  _lastY = 0;
  _lastUnderrunReport = 0;
  _lastUnderrunCount = 0;
  _cumulativeMs = 0;
  PlaybackPositionMs = 0;
  _lap.clear();
}

/**
  @brief Appends the points of one segment (excluding its start) to the current
         lap, interpolating so no step exceeds MAX_STEP, projection-mapping each
         emitted point. r/g/b = 0 means a blanked (laser-off) transit move.
*/
void ShowPlayerMode::appendSegment(int x0, int y0, int x1, int y1, byte r, byte g, byte b)
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
    // The last emitted point of a segment lands exactly on the target vertex
    // (a corner); hold it so the galvo settles before the next edge. Interior
    // interpolation points move on immediately.
    p.dwell = (i == steps) ? _cornerDwellTicks : 1;
    _lap.push_back(p);
  }
}

/**
  @brief Builds one full pass of the current frame into _lap: a blanked move to
         each path's start, then lit segments between the path's points. _lap is
         replayed (repeated) to fill the frame's time budget for flicker-free
         brightness. Sourced from the frame the ShowSource just streamed in.
*/
void ShowPlayerMode::buildLapFromCurrentFrame()
{
  _lap.clear();

  const std::vector<LasershowPoint> &points = _source->points();
  const std::vector<uint16_t> &pathLengths = _source->pathLengths();

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

  if (_lap.empty())
  {
    // A frame with nothing to draw still occupies its duration: hold the beam
    // blanked at the current position. This keeps gaps in a show (blink
    // effects) at their real length instead of being skipped, and guarantees
    // every frame pushes at least one point -- a looping show whose frames were
    // all empty previously spun in the refill loop forever and starved the
    // watchdog.
    int holdX = curX;
    int holdY = curY;
    _laser.projectionMap(holdX, holdY);

    OutPoint hold;
    hold.x = (int16_t)holdX;
    hold.y = (int16_t)holdY;
    hold.r = 0;
    hold.g = 0;
    hold.b = 0;
    hold.dwell = 1;
    _lap.push_back(hold);
  }

  // Total base-clock ticks one full pass of this lap occupies (points may dwell
  // more than one tick at corners), used to budget how many times the lap fits
  // in the frame's time.
  _lapTicks = 0;
  for (size_t i = 0; i < _lap.size(); i++)
  {
    _lapTicks += (_lap[i].dwell > 0) ? _lap[i].dwell : 1;
  }
}

void ShowPlayerMode::execute()
{
  preExecute();

  if (!_started)
  {
    resetState();
    _source = acquireSource();
    if (_source == nullptr)
    {
      return; // nothing to play yet
    }
    _started = true;

    // Honor the show's exported galvo speed (kpps), clamped to the safe range.
    // A missing/zero kpps (old file) falls back to the maximum, matching the
    // previous fixed-rate behavior.
    uint32_t kpps = _source->kpps();
    if (kpps == 0)
    {
      kpps = MAX_BASE_CLOCK_HZ;
    }
    _baseClockHz = constrain(kpps, MIN_BASE_CLOCK_HZ, MAX_BASE_CLOCK_HZ);

    // Corner hold converted to ticks at this show's point rate, at least 1.
    uint32_t cornerTicks = (CORNER_DWELL_US * _baseClockHz) / 1000000UL;
    _cornerDwellTicks = (cornerTicks > 0) ? (uint16_t)cornerTicks : 1;

    _player.start(_baseClockHz);
  }

  // A seek request from the API: jump the producer to the requested show time.
  if (SeekRequestMs >= 0)
  {
    uint32_t target = (uint32_t)SeekRequestMs;
    SeekRequestMs = -1;
    seekTo(target);
  }

  // Producer: keep the ring topped up. Each frame is drawn as a "lap" (one full
  // pass of the shape). The lap is repeated to fill the frame's tick budget so
  // the show plays at the correct wall-clock speed and stays bright. Crucially we
  // only advance to the next frame at a LAP BOUNDARY: a shape with more points
  // than the tick budget is still drawn IN FULL (it just takes a little longer)
  // instead of being cut off mid-shape. A lap is repeated only while a whole
  // further pass still fits in the remaining budget, so timing stays close to
  // nominal.
  while (_player.freeSpace() > REFILL_BATCH)
  {
    if (_needNewFrame)
    {
      if (!_source->readNextFrame())
      {
        if (!_loop)
        {
          _producingDone = true;
          break;
        }
        _source->rewind();
        _cumulativeMs = 0; // wrapped back to the start of the show
        if (!_source->readNextFrame())
        {
          _producingDone = true; // empty show; nothing to loop
          break;
        }
      }

      buildLapFromCurrentFrame();
      long ms = _source->currentDurationMs();
      _ticksLeftInCluster = ms * (long)_baseClockHz / 1000L;
      _lapIndex = 0;
      _needNewFrame = false;

      // Publish where we are on the show's timeline (start of this frame).
      PlaybackPositionMs = _cumulativeMs;
      _cumulativeMs += (uint32_t)ms;
    }

    const OutPoint &point = _lap[_lapIndex];
    _player.push(point);
    _ticksLeftInCluster -= (point.dwell > 0) ? point.dwell : 1;
    _lapIndex++;

    if (_lapIndex >= _lap.size())
    {
      _lapIndex = 0; // completed one full pass of the shape
      if (_ticksLeftInCluster < _lapTicks)
      {
        // Not enough budget left for another whole pass -> next frame.
        _needNewFrame = true;
      }
    }
  }

  reportUnderruns();

  // Finished once everything is produced AND the ring has drained.
  if (_producingDone && _player.queued() == 0)
  {
    _player.stop();
    _laser.setLaserPower(0, 0, 0);
    releaseSource();
    resetState();
    onProducingFinished();
  }
}

/**
  @brief Reports ring underruns at most once a second, and only when the count
         changed, so sustained producer starvation is visible without spam.
*/
void ShowPlayerMode::reportUnderruns()
{
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
}

/**
  @brief Jumps playback to targetMs on the show's timeline (frame granularity).
         The point clock is restarted to flush already-queued points, so the
         jump is heard immediately instead of after ~100 ms of old ring content
         drains; the restart blanks the laser for the instant of the jump,
         which is the safe direction.
*/
void ShowPlayerMode::seekTo(uint32_t targetMs)
{
  if (!_started || _source == nullptr)
  {
    return; // nothing playing; ignore the request
  }

  uint32_t frameStartMs = 0;
  if (_source->seekToTimeMs(targetMs, frameStartMs))
  {
    _cumulativeMs = frameStartMs;
    _producingDone = false; // seeking during the end-drain resumes playback
  }
  else
  {
    // Target at/beyond the end (or unreadable): wrap a looping show back to
    // its start; let a non-looping one finish.
    _source->rewind();
    _cumulativeMs = 0;
    _producingDone = !_loop;
  }

  _needNewFrame = true; // load the target frame on the next produce iteration
  PlaybackPositionMs = _cumulativeMs;
  _player.start(_baseClockHz); // flush points queued from the old position
}

void ShowPlayerMode::restart()
{
  _player.stop();
  _laser.setLaserPower(0, 0, 0);
  releaseSource();
  resetState();
}

void ShowPlayerMode::stop()
{
  _player.stop();
  _laser.setLaserPower(0, 0, 0);
  releaseSource();
  resetState();
}
