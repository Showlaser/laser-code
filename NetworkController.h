#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H
#include "GlobalConfig.h"
#include "Settings.h"
#include "Watchdog_t4.h"
#include <NativeEthernet.h>
#include <functional>
#include "SDCard.h"
#include "Laser.h"

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
  void init(WDT_T4<WDT1> &watchdog, SDCard &sdCard, Laser &laser);
  void sendBroadcast();
  void listenToApiCalls();
  void connectToController(byte controllerIp[4]);
  void disconnect();
  String getAssignedIP();
  ConnectionStatus getConnectionStatus();
  String generateUuid();
  void sendSettingsToController(settingsModel &settings);

private:
  WDT_T4<WDT1> _watchdog;
  SDCard _sdCard;
  // Pointer to THE Laser instance the realtime ISR uses, not a copy: the
  // emergency stop must set the output-disable latch on the real object or the
  // ISR would keep lighting the beam.
  Laser *_laser = nullptr;
  // Set by getRequestData while it streams a binary upload straight to SD;
  // read back by onSDCardBinaryUpload to build the response.
  bool _lastBinaryUploadSucceeded = false;
  std::vector<KeyValue> createDict();
  void setDoc(JsonDocument &doc, settingsModel &settings);
  String executeCallback(const String &httpMethod, const String &endPoint, IPAddress serverAddress, const String json);
  String onAdoptionRequest(IPAddress &serverAddress, const String &json);
  String onSettingsUpdate(IPAddress &serverAddress, const String &json);
  String onSettingsRequest(IPAddress &serverAddress, const String &json);
  String onIncommingAliveCheck(IPAddress &serverAddress, const String &json);
  String onSDCardFilesRequest(IPAddress &serverAddress, const String &json);
  String onSDCardBinaryUpload(IPAddress &serverAddress, const String &json);
  String onLiveShowUpload(IPAddress &serverAddress, const String &json);
  String onPlaySDCardFile(IPAddress &serverAddress, const String &json);
  String onStopPlayback(IPAddress &serverAddress, const String &json);
  String onEmergencyStop(IPAddress &serverAddress, const String &json);
  String onPlaybackStatusRequest(IPAddress &serverAddress, const String &json);
  String onSeekPlayback(IPAddress &serverAddress, const String &json);
  String onSDCardDeleteFile(IPAddress &serverAddress, const String &json);

  bool sendNetworkRequest(const String &httpMethod, const String &endPoint, IPAddress &serverAddress, const String &json);
  String ipToString(IPAddress ip);
  void teensyMAC(uint8_t *mac);
  void onApiCall(String type, String endPoint, IPAddress serverAddress, CallbackFunc cb);
  void getRequestData(EthernetClient &client, String &httpMethod, String &endPoint, String &json);
};

#endif