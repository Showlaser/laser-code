#ifndef LASERSHOWFILE_H
#define LASERSHOWFILE_H

#include "Arduino.h"
#include <SD.h>
#include <vector>

// A single point as stored in the ".lzs" binary: logical (-4000..4000) x/y and
// 0..255 per-channel colour (the firmware clamps colour to power on output).
struct LasershowPoint
{
  int16_t x;
  int16_t y;
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

/**
  @brief Streams a ".lzs" binary lasershow from the SD card one frame at a time.

  This replaces loading the whole show into a JsonDocument: only the header and
  the current frame ever live in RAM, so memory use is independent of show
  length. The file is little-endian, matching the Teensy, so fields are read
  straight into their types.

  Format:
    Header: char[4] "LZS1" | u16 version | u16 reserved | u32 kpps | u32 frameCount
    Frame : u16 durationMs | u16 pathCount
            per path: u16 pointCount | point[]: i16 x, i16 y, u8 r, u8 g, u8 b
*/
class LasershowFile
{
public:
  /**
    @brief Opens and validates a .lzs file. Returns false if it is missing or
           not a valid/supported .lzs (bad magic or version).
  */
  bool open(const String &filename);

  bool isOpen() const { return _open; }
  void close();

  /**
    @brief Reads the next frame into the reusable buffers. Returns false at the
           end of the show (or on a truncated/corrupt frame).
  */
  bool readNextFrame();

  /**
    @brief Seeks back to the first frame so the show can loop.
  */
  void rewind();

  uint32_t kpps() const { return _kpps; }
  uint32_t frameCount() const { return _frameCount; }

  // --- Valid after a successful readNextFrame() ---
  uint16_t currentDurationMs() const { return _durationMs; }
  // All points of the current frame, laid out flat, path after path.
  const std::vector<LasershowPoint> &points() const { return _points; }
  // Number of points in each path of the current frame, in order.
  const std::vector<uint16_t> &pathLengths() const { return _pathLengths; }

private:
  File _file;
  bool _open = false;
  uint32_t _kpps = 0;
  uint32_t _frameCount = 0;
  uint32_t _framesRead = 0;
  uint32_t _firstFrameOffset = 0;

  uint16_t _durationMs = 0;
  std::vector<LasershowPoint> _points;   // reused each frame (capacity retained)
  std::vector<uint16_t> _pathLengths;    // reused each frame

  // Reads sizeof(T) little-endian bytes straight into out. Returns false on a
  // short read (EOF/truncation).
  template <typename T>
  bool readRaw(T &out)
  {
    return _file.read(reinterpret_cast<uint8_t *>(&out), sizeof(T)) == static_cast<int>(sizeof(T));
  }
};

#endif
