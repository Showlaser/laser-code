#include <NativeEthernet.h>
#include "Laser.h"
#include "Settings.h"

Laser laser;
Settings settings;

EthernetClient _client;
IPAddress _server;

enum laserStatus {
  Defect = 0, // An hardware defect has been detected and the laser is locked due to safety reasons
  Ready = 1, // The laser is ready to receive and process commands that are received
  ConnectionToControllerLost = 2, // The laser has no connection to the controller
  NotConfigured = 3 // The laser is not yet configured
};

laserStatus _currentLaserStatus;

/**
 * Generate a MAC address for the Teensy
 * 
 * @param mac the variable too write the mac address to
 */
void teensyMAC(uint8_t *mac)
{
  for (uint8_t by = 0; by < 2; by++)
  {
    mac[by] = (HW_OCOTP_MAC1 >> ((1 - by) * 8)) & 0xFF;
  }
  for (uint8_t by = 0; by < 4; by++)
  {
    mac[by + 2] = (HW_OCOTP_MAC0 >> ((3 - by) * 8)) & 0xFF;
  }
} //TODO get output from function and put it in a constant variable

/*
* Sets the status of the laser and performs certain actions based on the status
* 
* @param status the status of the laser
*/
void setLaserStatus(laserStatus status) {
  _currentLaserStatus = status;

  if (status == laserStatus::Defect) {
    while(true) { // keep in a infinite loop so the laser is not reachable
      
    }
  }
  if (status == laserStatus::Ready) {
    laser.hardwareSelfCheck();
    // TODO do stuff with the led infront
  }
  if (status == laserStatus::ConnectionToControllerLost) {
    // TODO do stuff with the led infront
  }
  if (status == laserStatus::NotConfigured) {
    // TODO do stuff with the led infront and show menu on OLED screen
  }
}

/*
* This function tries to connect to the provided IP address on port 50000. After 10 tries the function calls the setLaserStatus function
*
* @param controllerIp the IP address of the controller to connect to
*/
void connectToController(String controllerIp) {
  char firstChar = controllerIp.charAt(0);

  if (firstChar == 255 || controllerIp.length() == 0) {
    setLaserStatus(laserStatus::NotConfigured);
    return;
  }

  unsigned int attempts = 0;
  while(!_client.connect(_server, 50000)) { // keep trying to connect to controller
    attempts++;
    if (attempts > 10) {
      setLaserStatus(laserStatus::ConnectionToControllerLost);
      return;
    }
  }
}

void setup() {
  byte mac[6];
  teensyMAC(mac);
  Ethernet.begin(mac);

  struct settingsModel = settings.getFromEEPROM();
  if (settingsModel.controllerIp.length() == 0) {
    setLaserStatus(laserstatus::NotConfigured);
  }

  laser.hardwareSelfCheck();
}

void loop() {
  if (_client.connected())
  {
    //digitalWrite(8, HIGH);
    //decodeCommands();
    //executeMessages();
  }
  else {
    //laser.turnLasersOff();
    _client.stop();
    setLaserStatus(laserStatus::ConnectionToControllerLost);
  }
}