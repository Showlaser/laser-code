#ifndef MEMORYSHOW_H
#define MEMORYSHOW_H

#include "Arduino.h"
#include <vector>
#include "ShowSource.h"

/**
  @brief A ShowSource backed by an in-RAM ".lzs" blob.

  Used for live pattern/animation playback: the PC uploads the whole (small) show
  as binary over the API, it is held in RAM here, and played frame-by-frame
  exactly like an SD show. The blob is moved in (not copied) to avoid a second
  copy in RAM. All ".lzs" parsing lives in ShowSource; this class only supplies
  the raw byte access over the buffer.
*/
class MemoryShow : public ShowSource
{
public:
  /**
    @brief Takes ownership of the blob (moved in) and parses/validates its
           header. Returns false if it is not a valid/supported .lzs.
  */
  bool load(std::vector<uint8_t> &&data);
  void close();

protected:
  bool readBytes(void *dst, size_t n) override;
  void seekTo(uint32_t offset) override;
  uint32_t position() override;

private:
  std::vector<uint8_t> _data;
  uint32_t _pos = 0;
};

#endif
