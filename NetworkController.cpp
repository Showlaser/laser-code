#include "NetworkController.h"
#include "Arduino.h"
#include <NativeEthernetUdp.h>
#include "Settings.h"
#include <ArduinoJson.h>

ConnectionStatus _connectionStatus = ConnectionStatus::NotConnected;

// EthernetUDP library https://github.com/arduino-libraries/Ethernet/blob/master/src/EthernetUdp.cpp
EthernetUDP _udpClient;
EthernetServer _server(80);

IPAddress _serverIP;
IPAddress broadcastIP(192, 168, 1, 255); // Broadcast address

unsigned int _udpPort = 8888;
unsigned int _tcpPort = 50000;

/**
  @brief Generate a MAC address for the Teensy

  @param mac the variable too write the mac address to
 */
void NetworkController::teensyMAC(uint8_t *mac)
{
  for (uint8_t by = 0; by < 2; by++)
  {
    mac[by] = (HW_OCOTP_MAC1 >> ((1 - by) * 8)) & 0xFF;
  }
  for (uint8_t by = 0; by < 4; by++)
  {
    mac[by + 2] = (HW_OCOTP_MAC0 >> ((3 - by) * 8)) & 0xFF;
  }
}

/**
  @brief This function tries to connect to the provided IP address on port 50000. After 10 tries the function calls the setLaserStatus function

  @param controllerIp the IP address of the controller to connect to
 */
void NetworkController::connectToController(byte controllerIp[4])
{
  byte firstChar = controllerIp[0];
  if (firstChar == 255)
  {
    return;
  }

  _serverIP = IPAddress(controllerIp[0], controllerIp[1], controllerIp[2], controllerIp[3]);
  EthernetClient client = _server.available();

  unsigned int attempts = 0;
  while (!client.connect(_serverIP, _tcpPort))
  { // keep trying to connect to controller
    _connectionStatus = ConnectionStatus::ConnectionPending;

    attempts++;
    if (attempts > 10)
    {
      _connectionStatus = ConnectionStatus::NotConnected;
      client.stop();
      return;
    }
  }

  _connectionStatus = ConnectionStatus::Connected;
}

String NetworkController::ipToString(IPAddress ip)
{
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

bool NetworkController::sendNetworkRequest(const String &httpMethod, const String &endPoint, IPAddress &serverAddress, const String &json)
{
  _watchdog.feed();
  Serial.println("Connection attempt to API");
  EthernetClient clientPost;

  const uint16_t serverPort = 5004;
  if (clientPost.connect(serverAddress, serverPort))
  {
    _watchdog.feed();
    Serial.println("Connected to API");

    clientPost.println(httpMethod + " " + endPoint + " HTTP/1.1");
    clientPost.print("Host: ");
    clientPost.println(serverAddress);
    clientPost.println("Content-Type: application/json");
    clientPost.print("Content-Length: ");
    clientPost.println(json.length());
    clientPost.println("Connection: close");
    clientPost.println();
    clientPost.println(json);

    return true;
  }
  else
  {
    Serial.println("Connection to API failed");
    return false;
  }
}

void NetworkController::init(WDT_T4<WDT1> &watchdog)
{
  _watchdog = watchdog;
  byte mac[6];
  teensyMAC(mac);
  Ethernet.begin(mac);
  _udpClient.begin(_udpPort);

  _server.begin();
  _watchdog.feed();
}

void NetworkController::onAdoptionRequest(IPAddress &serverAddress, const String &json)
{
  sendNetworkRequest("POST", "/laserconnection/connect", serverAddress, json);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  settingsModel settings = Settings::getSettings();

  strncpy(settings.uuid, doc["Uuid"] | "", sizeof(settings.uuid));
  settings.uuid[sizeof(settings.uuid) - 1] = '\0';

  strncpy(settings.laserName, doc["Name"] | "", sizeof(settings.laserName));
  settings.laserName[sizeof(settings.laserName) - 1] = '\0';

  settings.modelType = doc["ModelType"] | 0;
  settings.connectionStatus = doc["Status"] | 0;

  const char *ipStr = doc["IPAddress"] | "";
  if (strlen(ipStr) > 0)
  {
    int ip[4];
    if (sscanf(ipStr, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
    {
      for (int i = 0; i < 4; i++)
      {
        settings.controllerIp[i] = (byte)ip[i];
      }
    }
  }

  Settings::setSettings(settings);
}

void NetworkController::onApiCall(String type, String endPoint, IPAddress serverAddress, CallbackFunc cb)
{
  String json = "{ \"uuid\": \"96bb4468-6ccf-469e-a31b-e5b76d8d9950\" }";
  cb(serverAddress, json);
}

void NetworkController::getRequestData(EthernetClient &client, String &httpMethod, String &endPoint, String &json)
{
  String reqLine = "";
  bool headerEnded = false;
  int contentLength = 0;

  while (client.connected())
  {
    if (!client.available())
    {
      break;
    }

    char c = client.read();
    if (!headerEnded)
    {
      reqLine += c;

      if (reqLine.endsWith("\r\n\r\n"))
      {
        headerEnded = true;

        int indexOfEndpointStart = reqLine.indexOf(" /");
        int indexOfEndpointEnd = reqLine.indexOf(' ', indexOfEndpointStart + 1);
        endPoint = reqLine.substring(indexOfEndpointStart + 1, indexOfEndpointEnd);
        httpMethod = reqLine.substring(0, indexOfEndpointStart);

        int idx = reqLine.indexOf("Content-Length: ");
        if (idx > 0)
        {
          contentLength = reqLine.substring(idx + 16).toInt();
        }
      }
    }
    else
    {
      json += c;
      if ((int)json.length() >= contentLength)
      {
        break;
      }
    }
  }
}

std::vector<KeyValue> NetworkController::createDict()
{
  return {
      {"POST", "/adopt", [this](IPAddress serverAddress, const String json)
       {
         onAdoptionRequest(serverAddress, json);
       }}};
}

void NetworkController::executeCallback(const String &httpMethod, const String &endPoint, IPAddress serverAddress, const String json)
{
  static auto dict = createDict();
  for (auto &entry : dict)
  {
    if (entry.httpMethod == httpMethod && entry.endPoint == endPoint)
    {
      entry.cb(serverAddress, json);
      return;
    }
  }
  Serial.println("Geen match gevonden!");
}

void NetworkController::listenToApiCalls()
{
  EthernetClient client = _server.available();
  if (client)
  {
    IPAddress remoteIp = client.remoteIP();

    String json = "";
    String httpMethod = "";
    String endPoint = "";
    getRequestData(client, httpMethod, endPoint, json);

    if (json.length() > 0)
    {
      executeCallback(httpMethod, endPoint, remoteIp, json);
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println("{\"success\":true}");
    client.stop();
  }
}

void NetworkController::sendBroadcast()
{
  JsonDocument doc;
  doc["uuid"] = "895492a1-3ceb-40ba-a650-3de338f43e8b";
  doc["ip"] = ipToString(Ethernet.localIP());

  String jsonString;
  serializeJson(doc, jsonString);

  IPAddress broadcastIp(192, 168, 1, 255);
  _udpClient.beginPacket(broadcastIp, _udpPort);
  _udpClient.write(jsonString.c_str());
  _udpClient.endPacket();

  _watchdog.feed();
}

void NetworkController::disconnect()
{
  EthernetClient client = _server.available();
  client.stop();
}

ConnectionStatus NetworkController::getConnectionStatus()
{
  return _connectionStatus;
}