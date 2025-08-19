#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H
#include "GlobalConfig.h"
#include "Watchdog_t4.h"
#include <NativeEthernet.h>
#include <functional>

enum ConnectionStatus
{
  Connected = 0,
  ConnectionPending = 1,
  NotConnected = 2,
};

using CallbackFunc = std::function<void(String)>;
struct KeyValue
{
  String httpMethod;
  String endPoint;
  CallbackFunc cb;
};

class NetworkController
{
public:
  void init(WDT_T4<WDT1> &watchdog);
  void sendBroadcast();
  void listenToApiCalls();
  void NetworkController::disconnect();
  ConnectionStatus getConnectionStatus();

private:
  WDT_T4<WDT1> _watchdog;
  std::vector<KeyValue> createDict();
  void executeCallback(const String &httpMethod, const String &endPoint, const String json);
  void onAdoptionRequest(String json);
  String ipToString(IPAddress ip);
  void teensyMAC(uint8_t *mac);
  void connectToController(String controllerIp);
  void onApiCall(String type, String endpoint, CallbackFunc cb);
  void getRequestData(EthernetClient &client, String &httpMethod, String &endPoint, String &json);
};

#endif