#include "ShowPlayerMode.h"

// Maximum galvo step (in logical -4000..4000 units) between two emitted points.
// Smaller = smoother lines but more points per lap. This is the producer-side
// stand-in for the PC "maxStepPerTick" resample that arrives in a later phase.
static const int MAX_STEP = 200;

// How much headroom to keep before topping up, so each execute() pushes a batch
// rather than one point at a time.
static const uint16_t REFILL_BATCH = 64;

// Time to hold the beam at each path vertex (corner). A galvo cannot turn a
// sharp corner instantly; without a brief pause it "cuts the corner" and the
// shape rounds/bows. The hold is specified in TIME and converted to base-clock
// ticks per show, so corners settle equally long at every point rate.
static const uint32_t CORNER_DWELL_US = 200;

// Maximum galvo step per tick while the beam is BLANKED. Nobody sees the path
// of an invisible move, so it may step far coarser than a lit line -- but it
// must still STEP rather than jump outright: commanding a full-field jump in
// one tick leaves an (underdamped) galvo ringing at the destination, and the
// next lit point then paints a streak instead of a dot. Coarse guided steps
// arrive much calmer while costing only a fraction of the old fine
// interpolation (which dominated laps of scattered points and caused flicker).
static const int BLANK_STEP = 600;

// Settle time at the END of every blanked move, before the next lit point
// ignites. An underdamped galvo RINGS after arriving, and the ring's duration
// is set by the servo damping -- largely INDEPENDENT of the jump distance
// (short jumps ring almost as long, just with less amplitude) -- so this is a
// fixed time, not distance-scaled. Raise it until scattered dots stop
// streaking: the smallest such value measures the galvos' ring-out time.
// Well-damped scanners settle in a few hundred us; undertuned ones can need
// milliseconds, which physically caps how many scattered dots fit in a
// flicker-free lap -- the real cure for that is the damping trim, not software.
static const uint32_t BLANK_SETTLE_US = 800;

// Time to hold at the START of a blanked move, laser commanded off, BEFORE the
// mirrors begin to travel. The RGB DACs settle in microseconds, but the laser
// driver/diode chain takes far longer to actually go dark; departing
// immediately paints the departure path with the decaying beam (streaks
// instead of dots on scattered-point shows). The smallest value that removes
// the streaks is a direct measurement of the drivers' turn-off time.
static const uint32_t BLANK_OFF_SETTLE_US = 300;

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
         lap, projection-mapping each emitted point. Lit segments interpolate at
         MAX_STEP so the drawn line is straight. A blanked segment (r/g/b = 0)
         is an invisible transit: it first holds its departure position until
         the laser has gone dark, then steps coarsely (BLANK_STEP) and settles
         at the destination before the next lit point.
*/
void ShowPlayerMode::appendSegment(int x0, int y0, int x1, int y1, byte r, byte g, byte b)
{
  int dx = x1 - x0;
  int dy = y1 - y0;
  int dist = max(abs(dx), abs(dy));

  // Blanked moves step coarsely and settle longer at the end (see BLANK_STEP /
  // BLANK_SETTLE_US); lit segments step finely so the drawn line is straight,
  // and settle one corner hold.
  bool blanked = (r == 0 && g == 0 && b == 0);
  int stepSize = blanked ? BLANK_STEP : MAX_STEP;

  uint16_t endDwell = _cornerDwellTicks;
  if (blanked)
  {
    uint32_t settleTicks = (BLANK_SETTLE_US * _baseClockHz) / 1000000UL;
    if (settleTicks > _cornerDwellTicks)
    {
      endDwell = (uint16_t)settleTicks;
    }
  }

  if (blanked && dist > 0)
  {
    // Hold the departure position until the laser has actually gone dark
    // (see BLANK_OFF_SETTLE_US), so the decaying beam cannot paint the
    // departure path.
    uint32_t offTicks = (BLANK_OFF_SETTLE_US * _baseClockHz) / 1000000UL;
    if (offTicks > 0)
    {
      int holdX = x0;
      int holdY = y0;
      _laser.projectionMap(holdX, holdY);

      OutPoint hold;
      hold.x = (int16_t)holdX;
      hold.y = (int16_t)holdY;
      hold.r = 0;
      hold.g = 0;
      hold.b = 0;
      hold.dwell = (uint16_t)offTicks;
      _lap.push_back(hold);
    }
  }

  int steps = dist / stepSize;
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
    // The last emitted point of a segment lands exactly on the target: hold it
    // (corner hold, or the blank settle) so the galvo comes to rest before
    // what follows. Interior interpolation points move on immediately.
    p.dwell = (i == steps) ? endDwell : 1;
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
  // instead of being cut off mid-shape.
  //
  // _ticksLeftInCluster is a RUNNING signed budget: whatever a frame consumed
  // beyond (or left under) its nominal duration carries into the next frame
  // instead of being discarded. A heavy frame whose single lap overruns its
  // duration borrows time from the frames after it, and frames whose budget is
  // consumed entirely by that debt are SKIPPED -- like a video player dropping
  // frames -- so a heavy show finishes at its real wall-clock length instead of
  // stretching (10s instead of 5s), and a light show no longer runs slightly
  // fast from discarded remainders.
  while (_player.freeSpace() > REFILL_BATCH)
  {
    if (_needNewFrame)
    {
      // Consume frames until one still has budget left after settling the
      // carried debt; the ones in between are skipped without being drawn.
      bool frameReady = false;
      while (!frameReady)
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
          // A show that cannot run at real time at all would carry an ever-
          // growing debt and eventually skip entire passes; start each loop
          // pass with a clean slate instead.
          if (_ticksLeftInCluster < 0)
          {
            _ticksLeftInCluster = 0;
          }
          if (!_source->readNextFrame())
          {
            _producingDone = true; // empty show; nothing to loop
            break;
          }
        }

        long ms = _source->currentDurationMs();
        _ticksLeftInCluster += ms * (long)_baseClockHz / 1000L;

        // Publish where we are on the show's timeline (start of this frame).
        PlaybackPositionMs = _cumulativeMs;
        _cumulativeMs += (uint32_t)ms;

        // ">= 0" (not "> 0") so a zero-duration frame is still drawn: every
        // drawn frame pushes at least one point, guaranteeing progress even in
        // a show consisting only of such frames.
        frameReady = _ticksLeftInCluster >= 0;
      }
      if (_producingDone)
      {
        break;
      }

      buildLapFromCurrentFrame();
      _lapIndex = 0;
      _needNewFrame = false;
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
  @brief Once per second: reports the ACHIEVED point rate against the configured
         clock (well under _baseClockHz = the ISR cannot sustain the tick
         period, e.g. SPI time per tick), plus ring underruns when the count
         changed.
*/
void ShowPlayerMode::reportUnderruns()
{
  unsigned long now = millis();
  unsigned long elapsed = now - _lastUnderrunReport;
  if (elapsed < 1000)
  {
    return;
  }

  _lastUnderrunReport = now;
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

  _needNewFrame = true;    // load the target frame on the next produce iteration
  _ticksLeftInCluster = 0; // budget debt/credit from before the jump is meaningless now
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
