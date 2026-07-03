#ifndef SDCARD_H
#define SDCARD_H

#include "Arduino.h"
#include <SD.h>
#include <vector>

class SDCard
{
public:
  SDCard();

  /**
  @brief starts the SD card library
  @returns true if SD card is present and can be read, false if no card is present or a non supported filesystem is used. Supported filesystems are FAT16 and FAT32
  */
  bool init();

  /**
  @brief reads the content of the SD card and returns the file names as a string array. NOTE only .lzs files are supported and read
  @returns the content of the SD card
  */
  std::vector<String> getLzsFiles();

  /**
  @brief delete the lzs file from the SD card
  @param fileName the name of the file to delete
  */
  bool deleteLzsFile(String fileName);

  /**
  @brief opens a file for writing, truncating any existing file with the same
         name (unlike FILE_WRITE which appends). Used to stream an uploaded
         binary show straight to disk without buffering it in RAM.
  @param fileName the name of the file to (re)create
  @returns the opened File; test it with `if (file)` for success
  */
  File openForWrite(const String &fileName);

private:
};

#endif