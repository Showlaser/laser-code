#include "NetworkController.h"
#include "Arduino.h"
#include <NativeEthernetUdp.h>

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
void NetworkController::connectToController(String controllerIp)
{
  char firstChar = controllerIp.charAt(0);
  if (firstChar == 255 || controllerIp.length() == 0)
  {
    return;
  }

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
}

String NetworkController::ipToString(IPAddress ip)
{
  return String(ip[0]) + "." +
         String(ip[1]) + "." +
         String(ip[2]) + "." +
         String(ip[3]);
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

void NetworkController::onAdoptionRequest(String json)
{
  Serial.println("ONAdopt");
}

void NetworkController::onApiCall(String type, String endpoint, CallbackFunc cb)
{
  String json = "{ \"uuid\": \"96bb4468-6ccf-469e-a31b-e5b76d8d9950\" }";
  cb(json);
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
      if (json.length() >= contentLength)
      {
        break;
      }
    }
  }
}

std::vector<KeyValue> NetworkController::createDict()
{
  return {
      {"POST", "/adopt", [this](String json)
       { onAdoptionRequest(json); }}};
}

void NetworkController::executeCallback(const String &httpMethod, const String &endPoint, const String json)
{
  static auto dict = createDict();
  for (auto &entry : dict)
  {
    if (entry.httpMethod == httpMethod && entry.endPoint == endPoint)
    {
      entry.cb(json);
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
      executeCallback(httpMethod, endPoint, json);

      JsonDocument doc;
      if (deserializeJson(doc, json) == DeserializationError::Ok)
      {
        const char *uuid = doc["Uuid"];
        const char *name = doc["Name"];
        const char *ip = doc["IPAddress"];

        Serial.printf("Laser registered: %s (%s) @ %s\n", name, uuid, ip);
      }
      else
      {
        Serial.println("Invalid JSON");
      }
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