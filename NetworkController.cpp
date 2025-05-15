#include "NetworkController.h"
#include "Arduino.h"
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>

ConnectionStatus _connectionStatus = ConnectionStatus::NotConnected;

// EthernetUDP library https://github.com/arduino-libraries/Ethernet/blob/master/src/EthernetUdp.cpp
EthernetUDP _udpClient;
EthernetServer _server;

IPAddress _serverIP;
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
    return;
  }

  EthernetClient client = _server.available();

  unsigned int attempts = 0;
  while (!client.connect(_serverIP, _tcpPort)) {  // keep trying to connect to controller
    _connectionStatus = ConnectionStatus::ConnectionPending;

    attempts++;
    if (attempts > 10) {
      _connectionStatus = ConnectionStatus::NotConnected;
      client.stop();
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
  _udpClient.beginPacket(IPAddress(255, 255, 255, 255), _udpPort);  // Broadcast
  _udpClient.print("Teensy_Alive:192.168.1.100");  // Include IP
  _udpClient.endPacket();
}

void NetworkController::disconnect() {
  EthernetClient client = _server.available();
  client.stop();
}

ConnectionStatus NetworkController::getConnectionStatus() {
  return _connectionStatus;
}