#include "SDCard.h"
#include <SD.h>
#include <SPI.h>
#include <vector>

// Use SPI2 and specify pins
#define SD_MOSI 35
#define SD_MISO 34
#define SD_SCK 28

const int chipSelect = BUILTIN_SDCARD;

SDCard::SDCard()
{
}

bool SDCard::init()
{
  SPI.setMOSI(SD_MOSI);
  SPI.setSCK(SD_SCK);
  SPI.setMISO(SD_MISO);
  return SD.begin(chipSelect);
}

std::vector<String> SDCard::getLzsFiles()
{
  std::vector<String> items;

  File root = SD.open("/");
  while (true)
  {
    File entry = root.openNextFile();

    if (!entry)
    {
      entry.close();
      break; // no more files
    }

    if (!entry.isDirectory() && (strstr(entry.name(), ".lzs")))
    {
      items.push_back(entry.name());
    }

    entry.close();
  }

  return items;
}

bool SDCard::deleteLzsFile(String fileName)
{
  if (!SD.exists(fileName.c_str()) || !fileName.endsWith(".lzs"))
  {
    return false;
  }

  return SD.remove(fileName.c_str());
}

File SDCard::openForWrite(const String &fileName)
{
  // FILE_WRITE appends, so remove any existing file first to overwrite cleanly.
  if (SD.exists(fileName.c_str()))
  {
    SD.remove(fileName.c_str());
  }

  return SD.open(fileName.c_str(), FILE_WRITE);
}