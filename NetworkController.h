#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H
#include "GlobalConfig.h"
#include "Settings.h"
#include "Watchdog_t4.h"
#include <NativeEthernet.h>
#include <functional>
#include "SDCard.h"

enum ConnectionStatus
{
  Connected = 0,
  ConnectionPending = 1,
  NotConnected = 2,
  NoNetworkCableConnected = 3
};

// A callback handles an incoming API call and returns the JSON body to send
// back to the caller. Return an empty String to fall back to the default
// {"success":true} response.
using CallbackFunc = std::function<String(IPAddress, String)>;
struct KeyValue
{
  String httpMethod;
  String endPoint;
  CallbackFunc cb;
};

class NetworkController
{
public:
  bool laserControllerAliveCheck();
  void init(WDT_T4<WDT1> &watchdog, SDCard &sdCard);
  void sendBroadcast();
  void listenToApiCalls();
  void connectToController(byte controllerIp[4]);
  String onIncommingAliveCheck(IPAddress &serverAddress, const String &json);
  String onSDCardFilesRequest(IPAddress &serverAddress, const String &json);
  String onSDCardReadJsonFile(IPAddress &serverAddress, const String &json);
  String onSDCardCreateJsonFile(IPAddress &serverAddress, const String &json);
  String onSDCardDeleteJsonFile(IPAddress &serverAddress, const String &json);
  void disconnect();
  String getAssignedIP();
  ConnectionStatus getConnectionStatus();
  String generateUuid();
  void sendSettingsToController(settingsModel &settings);

private:
  WDT_T4<WDT1> _watchdog;
  SDCard _sdCard;
  std::vector<KeyValue> createDict();
  void setDoc(JsonDocument &doc, settingsModel &settings);
  String executeCallback(const String &httpMethod, const String &endPoint, IPAddress serverAddress, const String json);
  String onAdoptionRequest(IPAddress &serverAddress, const String &json);
  String onSettingsUpdate(IPAddress &serverAddress, const String &json);
  String onSettingsRequest(IPAddress &serverAddress, const String &json);
  bool sendNetworkRequest(const String &httpMethod, const String &endPoint, IPAddress &serverAddress, const String &json);
  String ipToString(IPAddress ip);
  void teensyMAC(uint8_t *mac);
  void onApiCall(String type, String endPoint, IPAddress serverAddress, CallbackFunc cb);
  void getRequestData(EthernetClient &client, String &httpMethod, String &endPoint, String &json);
};

#endif