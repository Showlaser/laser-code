#ifndef SDCARD_H
#define SDCARD_H

#include "Arduino.h"
#include <vector>

class SDCard {
public:
  SDCard();

  /**
  @brief starts the SD card library
  @returns true if SD card is present and can be read, false if no card is present or a non supported filesystem is used. Supported filesystems are FAT16 and FAT32
  */
  bool init();

  /**
  @brief reads the content of the SD card and returns the file names as a string array. NOTE only .json files are supported and read
  @returns the content of the SD card
  */
  std::vector<String> getJsonFiles();

  /**
  @brief reads the content of the json file and returns the file content as a string
  @param fileName the filename to read
  @returns the content of the file in string format. If the file is empty a empty string is returned
  */
  String readJsonFile(String fileName);

  /**
  @brief write the json string to the SD card
  @param json the json string to save on the SD card
  @param fileName the name of the file to create
  */
  bool createJsonFile(String json, String fileName);

  /**
  @brief delete the json file from the SD card
  @param fileName the name of the file to delete
  */
  bool deleteJsonFile(String fileName);
private:
};

#endif