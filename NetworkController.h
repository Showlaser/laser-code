#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H
#include "GlobalConfig.h"

enum ConnectionStatus {
  Connected = 0,
  ConnectionPending = 1,
  NotConnected = 2,
};

class NetworkController {
  public:
    void init();
    void sendBroadcast();
    void disconnect();
    ConnectionStatus getConnectionStatus();

  private:
    void teensyMAC(uint8_t* mac);
    void connectToController(String controllerIp);
};

#endif