#ifndef LASERSHOWFILE_H
#define LASERSHOWFILE_H

#include "Arduino.h"
#include <SD.h>
#include "ShowSource.h"

/**
  @brief A ShowSource backed by an ".lzs" file on the SD card.

  Only the header and the current frame ever live in RAM, so memory use is
  independent of show length. SD (SDIO) is a separate bus from the DAC SPI, so
  streaming here is safe during playback. All ".lzs" parsing lives in ShowSource;
  this class only supplies the raw file byte access.
*/
class LasershowFile : public ShowSource
{
public:
  /**
    @brief Opens and validates a .lzs file. Returns false if it is missing or
           not a valid/supported .lzs (bad magic or version).
  */
  bool open(const String &filename);
  void close();

protected:
  bool readBytes(void *dst, size_t n) override;
  void seekTo(uint32_t offset) override;
  uint32_t position() override;

private:
  File _file;
};

#endif
