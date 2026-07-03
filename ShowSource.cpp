#include "ShowSource.h"

bool ShowSource::parseHeader()
{
  seekTo(0);

  char magic[4];
  if (!readBytes(magic, 4) ||
      magic[0] != 'L' || magic[1] != 'Z' || magic[2] != 'S' || magic[3] != '1')
  {
    resetParsed();
    return false;
  }

  uint16_t version = 0;
  uint16_t reserved = 0;
  if (!readRaw(version) || !readRaw(reserved) || version != 1)
  {
    resetParsed();
    return false;
  }

  if (!readRaw(_kpps) || !readRaw(_frameCount))
  {
    resetParsed();
    return false;
  }

  _firstFrameOffset = position();
  _framesRead = 0;
  _open = true;
  return true;
}

void ShowSource::resetParsed()
{
  _open = false;
  _kpps = 0;
  _frameCount = 0;
  _framesRead = 0;
  _firstFrameOffset = 0;
  _durationMs = 0;
}

void ShowSource::rewind()
{
  if (_open)
  {
    seekTo(_firstFrameOffset);
    _framesRead = 0;
  }
}

bool ShowSource::seekToTimeMs(uint32_t targetMs, uint32_t &frameStartMs)
{
  if (!_open)
  {
    return false;
  }

  rewind();
  uint32_t cumulative = 0;

  while (_framesRead < _frameCount)
  {
    uint32_t frameOffset = position();

    uint16_t durationMs = 0;
    uint16_t pathCount = 0;
    if (!readRaw(durationMs) || !readRaw(pathCount))
    {
      return false; // truncated
    }

    if (cumulative + durationMs > targetMs)
    {
      // This frame contains the target; rewind to its header so the caller's
      // next readNextFrame() plays it.
      seekTo(frameOffset);
      frameStartMs = cumulative;
      return true;
    }

    // Skip this frame's point data without materializing it (a point is
    // i16 x, i16 y, u8 r, u8 g, u8 b = 7 bytes).
    for (uint16_t pi = 0; pi < pathCount; pi++)
    {
      uint16_t pointCount = 0;
      if (!readRaw(pointCount))
      {
        return false;
      }
      seekTo(position() + (uint32_t)pointCount * 7u);
    }

    cumulative += durationMs;
    _framesRead++;
  }

  return false; // target lies beyond the end of the show
}

bool ShowSource::readNextFrame()
{
  if (!_open || _framesRead >= _frameCount)
  {
    return false;
  }

  uint16_t pathCount = 0;
  if (!readRaw(_durationMs) || !readRaw(pathCount))
  {
    return false; // truncated
  }

  _points.clear();
  _pathLengths.clear();

  for (uint16_t pi = 0; pi < pathCount; pi++)
  {
    uint16_t pointCount = 0;
    if (!readRaw(pointCount))
    {
      return false;
    }
    _pathLengths.push_back(pointCount);

    for (uint16_t j = 0; j < pointCount; j++)
    {
      LasershowPoint p;
      if (!readRaw(p.x) || !readRaw(p.y) ||
          !readRaw(p.r) || !readRaw(p.g) || !readRaw(p.b))
      {
        return false;
      }
      _points.push_back(p);
    }
  }

  _framesRead++;
  return true;
}
