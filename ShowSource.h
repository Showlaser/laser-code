#ifndef SHOWSOURCE_H
#define SHOWSOURCE_H

#include "Arduino.h"
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
  @brief Streams an ".lzs" binary lasershow one frame at a time, independent of
         where the bytes come from (SD card, RAM, ...).

  Only the header and the current frame ever live in RAM, so memory use is
  independent of show length. The format is little-endian, matching the Teensy,
  so fields are read straight into their types. Subclasses provide the raw byte
  access (readBytes/seekTo/position); this base owns the format parsing so there
  is a single definition of the layout.

  Format:
    Header: char[4] "LZS1" | u16 version | u16 reserved | u32 kpps | u32 frameCount
    Frame : u16 durationMs | u16 pathCount
            per path: u16 pointCount | point[]: i16 x, i16 y, u8 r, u8 g, u8 b
*/
class ShowSource
{
public:
  virtual ~ShowSource() {}

  bool isOpen() const { return _open; }

  /**
    @brief Reads the next frame into the reusable buffers. Returns false at the
           end of the show (or on a truncated/corrupt frame).
  */
  bool readNextFrame();

  /**
    @brief Seeks back to the first frame so the show can loop.
  */
  void rewind();

  /**
    @brief Positions the stream so the next readNextFrame() returns the frame
           containing targetMs (frame start times are the cumulative frame
           durations). Frames are skipped by seeking past their point data, so
           this is fast even for long shows. Returns false when targetMs lies
           at/beyond the end of the show (the stream is then at an undefined
           frame; rewind before reading). On success frameStartMs holds the
           found frame's start time on the show's timeline.
  */
  bool seekToTimeMs(uint32_t targetMs, uint32_t &frameStartMs);

  uint32_t kpps() const { return _kpps; }
  uint32_t frameCount() const { return _frameCount; }

  // --- Valid after a successful readNextFrame() ---
  uint16_t currentDurationMs() const { return _durationMs; }
  // All points of the current frame, laid out flat, path after path.
  const std::vector<LasershowPoint> &points() const { return _points; }
  // Number of points in each path of the current frame, in order.
  const std::vector<uint16_t> &pathLengths() const { return _pathLengths; }

protected:
  // Parses and validates the LZS1 header from byte offset 0, positioning the
  // source at the first frame. Returns false on bad magic/version or a short
  // read; sets _open on success. Subclasses call this once the byte source is
  // ready.
  bool parseHeader();

  // Clears parsed state. Subclasses call this from their close()/reset.
  void resetParsed();

  // --- Raw byte access, provided by subclasses ---
  // Reads exactly n bytes into dst, advancing the cursor. Returns false on a
  // short read (EOF/truncation).
  virtual bool readBytes(void *dst, size_t n) = 0;
  // Moves the read cursor to an absolute byte offset.
  virtual void seekTo(uint32_t offset) = 0;
  // Returns the current absolute read cursor.
  virtual uint32_t position() = 0;

  bool _open = false;
  uint32_t _kpps = 0;
  uint32_t _frameCount = 0;
  uint32_t _framesRead = 0;
  uint32_t _firstFrameOffset = 0;

  uint16_t _durationMs = 0;
  std::vector<LasershowPoint> _points; // reused each frame (capacity retained)
  std::vector<uint16_t> _pathLengths;  // reused each frame

  // Reads sizeof(T) little-endian bytes straight into out.
  template <typename T>
  bool readRaw(T &out)
  {
    return readBytes(&out, sizeof(T));
  }
};

#endif
