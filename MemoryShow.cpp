#include "MemoryShow.h"
#include <string.h>
#include <utility>

bool MemoryShow::load(std::vector<uint8_t> &&data)
{
  close();
  _data = std::move(data);
  _pos = 0;

  if (!parseHeader())
  {
    close();
    return false;
  }

  return true;
}

void MemoryShow::close()
{
  _data.clear();
  _data.shrink_to_fit();
  _pos = 0;
  resetParsed();
}

bool MemoryShow::readBytes(void *dst, size_t n)
{
  if (_pos + n > _data.size())
  {
    return false;
  }
  memcpy(dst, _data.data() + _pos, n);
  _pos += n;
  return true;
}

void MemoryShow::seekTo(uint32_t offset)
{
  _pos = offset;
}

uint32_t MemoryShow::position()
{
  return _pos;
}
