#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H

class AudienceShutter {
  public:
    void init();
    void sendBroadcast();
    void disconnect();
    ConnectionStatus getConnectionStatus();

  private:

};

#endif