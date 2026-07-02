#include "LasershowFile.h"

bool LasershowFile::open(const String &filename)
{
  close();

  _file = SD.open(filename.c_str());
  if (!_file)
  {
    return false;
  }

  if (!parseHeader())
  {
    close();
    return false;
  }

  return true;
}

void LasershowFile::close()
{
  if (_file)
  {
    _file.close();
  }
  resetParsed();
}

bool LasershowFile::readBytes(void *dst, size_t n)
{
  return _file.read(reinterpret_cast<uint8_t *>(dst), n) == static_cast<int>(n);
}

void LasershowFile::seekTo(uint32_t offset)
{
  _file.seek(offset);
}

uint32_t LasershowFile::position()
{
  return (uint32_t)_file.position();
}
