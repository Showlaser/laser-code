#include <NativeEthernet.h>
#include "Laser.h"
#include <EEPROM.h>
#include <ArduinoJson.h>  // include before MsgPacketizer.h
#include <queue>
#include "OledModule.h"

Laser _laser;
OledModule _oledModule;

EthernetClient _client;
IPAddress _server;

enum laserStatus {
  Defect = 0,                      // An hardware defect has been detected and the laser is locked due to safety reasons
  Ready = 1,                       // The laser is ready to receive and process commands that are received
  ConnectionToControllerLost = 2,  // The laser has no connection to the controller
  NotConfigured = 3                // The laser is not yet configured
};

laserStatus _currentLaserStatus;

/**
  @brief Generate a MAC address for the Teensy
 
  @param mac the variable too write the mac address to
 */
void teensyMAC(uint8_t* mac) {
  for (uint8_t by = 0; by < 2; by++) {
    mac[by] = (HW_OCOTP_MAC1 >> ((1 - by) * 8)) & 0xFF;
  }
  for (uint8_t by = 0; by < 4; by++) {
    mac[by + 2] = (HW_OCOTP_MAC0 >> ((3 - by) * 8)) & 0xFF;
  }
}  // TODO get output from function and put it in a constant variable

/**
 @brief Sets the status of the laser and performs certain actions based on the status
 
 @param status the status of the laser
 */
void setLaserStatus(laserStatus status) {
  _currentLaserStatus = status;

  if (status == laserStatus::Defect) {
    while (true) {  // keep in a infinite loop so the laser is not reachable
    }
  }
  if (status == laserStatus::Ready) {
    _laser.hardwareSelfCheck();
    // TODO do stuff with the led infront
  }
  if (status == laserStatus::ConnectionToControllerLost) {
    // TODO do stuff with the led infront
  }
  if (status == laserStatus::NotConfigured) {
    // TODO do stuff with the led infront and show menu on OLED screen
  }
}

/**
  @brief This function tries to connect to the provided IP address on port 50000. After 10 tries the function calls the setLaserStatus function
 
  @param controllerIp the IP address of the controller to connect to
 */
void connectToController(String controllerIp) {
  char firstChar = controllerIp.charAt(0);

  if (firstChar == 255 || controllerIp.length() == 0) {
    setLaserStatus(laserStatus::NotConfigured);
    return;
  }

  unsigned int attempts = 0;
  while (!_client.connect(_server, 50000)) {  // keep trying to connect to controller
    attempts++;
    if (attempts > 10) {
      setLaserStatus(laserStatus::ConnectionToControllerLost);
      return;
    }
  }
}

struct settingsModel {
  byte controllerIp[4];
  byte maxPowerRgb[3];
};

/**
 @brief Checks if the array objects in the array are not empty

 @param array the array to check
 @return true if the objects in the array do not have a default value, false if the the objects in the array have a default value
*/
bool checkIfArrayIsNotEmpty(byte array[]) {
  int emptyValueOccurances = 0;
  const unsigned int lengthOfArray = sizeof(byte) / sizeof(array[0]);
  for (unsigned int i = 0; i < lengthOfArray; i++) {
    if (i == 255) {
      emptyValueOccurances++;
    }
  }

  return emptyValueOccurances != lengthOfArray;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Start");
  _oledModule.init();
  while (true) {
    _oledModule.checkForInput();
  }

  byte mac[6];
  teensyMAC(mac);
  Ethernet.begin(mac);

  settingsModel model;
  EEPROM.get(0, model);

  bool controllerIpIsNotEmpty = checkIfArrayIsNotEmpty(model.controllerIp);
  bool maxPowerIpIsNotEmpty = checkIfArrayIsNotEmpty(model.maxPowerRgb);
  bool settingsValid = controllerIpIsNotEmpty && maxPowerIpIsNotEmpty;
  if (!settingsValid) {
    setLaserStatus(laserStatus::NotConfigured);
  }

  _laser.hardwareSelfCheck();
}

void loop() {
  if (_client.connected()) {
  } else {
    // laser.turnLasersOff();
    _client.stop();
    setLaserStatus(laserStatus::ConnectionToControllerLost);
  }
}