#include "NetworkController.h"
#include "Arduino.h"
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>

EthernetUDP _udpClient;
EthernetServer _server;

IPAddress _server;
IPAddress broadcastIP(192, 168, 1, 255); // Broadcast address

unsigned int _udpPort = 8888;
unsigned int _tcpPort = 50000;

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
  while (!_client.connect(_server, _tcpPort)) {  // keep trying to connect to controller
    attempts++;
    if (attempts > 10) {
      setLaserStatus(laserStatus::ConnectionToControllerLost);
      return;
    }
  }
}

void NetworkController::init() {
  byte mac[6];
  teensyMAC(mac);
  
  Ethernet.begin(mac);
  _udpClient.begin(_udpPort);
  _server.begin();
}

void NetworkController::sendBroadcast() {
  if (!client.connected()) {
    return;
  }


}

void NetworkController::disconnect() {
}

ConnectionStatus NetworkController::getConnectionStatus() {
}