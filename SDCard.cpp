#include "SDCard.h"
#include <SD.h>
#include <SPI.h>
#include <vector>

// Use SPI2 and specify pins
#define SD_MOSI 35
#define SD_MISO 34
#define SD_SCK 28

const int chipSelect = BUILTIN_SDCARD;

SDCard::SDCard() {
}

bool SDCard::init() {
  SPI.setMOSI(SD_MOSI);
  SPI.setSCK(SD_SCK);
  SPI.setMISO(SD_MISO);
  return SD.begin(chipSelect);
}

std::vector<String> SDCard::getJsonFiles() {
  std::vector<String> items;

  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();

    if (!entry) {
      entry.close();
      break;  // no more files
    }

    if (!entry.isDirectory() && strstr(entry.name(), ".json")) {
      items.push_back(entry.name());
    }

    entry.close();
  }

  return items;
}

String SDCard::readJsonFile(String fileName) {
  if (!fileName.endsWith(".json")) {
    return "";
  }

  File jsonFile = SD.open(fileName.c_str());
  if (!jsonFile) {
    return "";
  }

  String result;
  while (jsonFile.available()) {
    char c = jsonFile.read();
    result += c;

    if (c == 0) {
      break;
    }
  }

  jsonFile.close();
  return result;
}

bool SDCard::createJsonFile(String json, String fileName) {
  if (!fileName.endsWith(".json") || SD.exists(fileName.c_str())) {
    return false;
  }

  File jsonFile = SD.open(fileName.c_str(), FILE_WRITE);
  if (jsonFile) {
    jsonFile.println(json);
    jsonFile.close();
    return true;
  }

  jsonFile.close();
  return false;
}

bool SDCard::deleteJsonFile(String fileName) {
  if (!SD.exists(fileName.c_str()) || !fileName.endsWith(".json")) {
    return false;
  }

  return SD.remove(fileName.c_str());
}