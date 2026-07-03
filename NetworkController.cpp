#include "NetworkController.h"
#include "Arduino.h"
#include <NativeEthernetUdp.h>
#include "Settings.h"
#include <ArduinoJson.h>
#include <vector>

ConnectionStatus _connectionStatus = ConnectionStatus::NotConnected;

// EthernetUDP library https://github.com/arduino-libraries/Ethernet/blob/master/src/EthernetUdp.cpp
EthernetUDP _udpClient;
EthernetServer _server(80);

IPAddress _serverIP;
IPAddress broadcastIP(192, 168, 1, 255); // Broadcast address

unsigned int _udpPort = 8888;
unsigned int _tcpPort = 5004;

// Upper bound on a live pattern/animation upload held in RAM. Patterns and
// typical animations are far smaller; this rejects anything lasershow-sized so a
// live upload can never starve controller RAM.
static const uint32_t MAX_LIVE_SHOW_BYTES = 128UL * 1024UL;

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

bool NetworkController::laserControllerAliveCheck()
{
  settingsModel settings = Settings::getSettings();
  bool alive = sendNetworkRequest("GET", String("/laserconnection/alive?uuid=") + settings.uuid, _serverIP, "");
  Serial.println("Alive check");
  Serial.println(alive ? "Alive" : "Not alive");

  if (alive && _connectionStatus != ConnectionStatus::Connected)
  {
    _connectionStatus = ConnectionStatus::Connected;
    connectToController(settings.controllerIp);
  }
  else if (alive)
  {
    _connectionStatus = ConnectionStatus::Connected;
  }
  else
  {
    _connectionStatus = ConnectionStatus::NotConnected;
  }
  return alive;
}

void NetworkController::setDoc(JsonDocument &doc, settingsModel &settings)
{
  doc["Uuid"] = settings.uuid;
  doc["Name"] = settings.name;
  doc["ModelType"] = settings.modelType;
  doc["IPAddress"] = ipToString(Ethernet.localIP());
  doc["Status"] = 1;
  doc["MaxPowerPerlaserInPercentage"] = settings.maxPowerPerlaserInPercentage;
  doc["ProjectionTopInPercentage"] = settings.projectionTopInPercentage;
  doc["ProjectionBottomInPercentage"] = settings.projectionBottomInPercentage;
  doc["ProjectionLeftInPercentage"] = settings.projectionLeftInPercentage;
  doc["ProjectionRightInPercentage"] = settings.projectionRightInPercentage;
}

/**
  @brief Serialize the current settings and push them to the connected controller
         via PUT /laserconnection/.

  @param settings the settings to send to the controller
 */
void NetworkController::sendSettingsToController(settingsModel &settings)
{
  JsonDocument doc;
  setDoc(doc, settings);

  String jsonString;
  serializeJson(doc, jsonString);
  sendNetworkRequest("PUT", String("/laserconnection/"), _serverIP, jsonString);
}

/**
  @brief This function tries to connect to the provided IP address on port 50000. After 10 tries the function calls the setLaserStatus function

  @param controllerIp the IP address of the controller to connect to
 */
void NetworkController::connectToController(byte controllerIp[4])
{
  _watchdog.feed();

  byte firstChar = controllerIp[0];
  if (firstChar == 255 || firstChar == 0)
  {
    return;
  }

  Serial.println("Trying to connect to controller at " + String(controllerIp[0]) + "." + String(controllerIp[1]) + "." + String(controllerIp[2]) + "." + String(controllerIp[3]));
  _serverIP = IPAddress(controllerIp[0], controllerIp[1], controllerIp[2], controllerIp[3]);

  settingsModel settings = Settings::getSettings();
  JsonDocument doc;
  setDoc(doc, settings);

  String jsonString;

  serializeJson(doc, jsonString);
  _watchdog.feed();

  // The very first TCP connect after boot has to ARP-resolve the controller
  // before it can send the SYN. With an empty ARP cache (and a switch port that
  // just came up) that round trip often exceeds the deliberately low 1500ms
  // connect timeout, so the first attempt fails while later ones succeed. Retry
  // a few times to cover the ARP warm-up; each attempt stays well under the 10s
  // watchdog and we feed it between tries.
  const int maxConnectAttempts = 3;
  bool success = false;
  for (int attempt = 1; attempt <= maxConnectAttempts && !success; attempt++)
  {
    success = sendNetworkRequest("POST", "/laserconnection/connect", _serverIP, jsonString);
    if (!success && attempt < maxConnectAttempts)
    {
      Serial.println("Connect attempt " + String(attempt) + " failed, retrying");
      _watchdog.feed();
    }
  }

  if (success)
  {
    _connectionStatus = ConnectionStatus::Connected;
  }
  else
  {
    _connectionStatus = ConnectionStatus::NotConnected;
  }
}

String NetworkController::ipToString(IPAddress ip)
{
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

/**
  @brief Generates a random RFC 4122 version-4 UUID in canonical
  8-4-4-4-12 lowercase hex format (e.g. "895492a1-3ceb-40ba-a650-3de338f43e8b").

  @return the generated UUID as a String
 */
String NetworkController::generateUuid()
{
  // Seed once with hardware entropy: the chip's unique OCOTP fuse values (unique
  // per Teensy) mixed with timing and floating-pin analog noise, so different
  // boards and different boots produce different UUIDs.
  static bool seeded = false;
  if (!seeded)
  {
    uint32_t seed = HW_OCOTP_MAC0 ^ HW_OCOTP_MAC1 ^ micros() ^ ((uint32_t)analogRead(A0) << 16);
    randomSeed(seed);
    seeded = true;
  }

  uint8_t bytes[16];
  for (int i = 0; i < 16; i++)
  {
    bytes[i] = (uint8_t)random(256);
  }

  // Set the version (4) and variant (RFC 4122) bits.
  bytes[6] = (bytes[6] & 0x0F) | 0x40;
  bytes[8] = (bytes[8] & 0x3F) | 0x80;

  char buf[37];
  snprintf(buf, sizeof(buf),
           "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
           bytes[0], bytes[1], bytes[2], bytes[3],
           bytes[4], bytes[5], bytes[6], bytes[7],
           bytes[8], bytes[9], bytes[10], bytes[11],
           bytes[12], bytes[13], bytes[14], bytes[15]);

  return String(buf);
}

bool NetworkController::sendNetworkRequest(const String &httpMethod, const String &endPoint, IPAddress &serverAddress, const String &json)
{
  _watchdog.feed();
  Serial.println("Sending network request: httpMethod: " + httpMethod + " endPoint: " + endPoint + " serverAddress: " + ipToString(serverAddress));
  EthernetClient clientPost;

  // Bound the blocking connect() well under the watchdog timeout (10s). connect()
  // does not feed our watchdog while it polls, so this value must stay low. 1500ms
  // is enough for a real handshake (incl. first ARP) but far below the watchdog.
  // NOTE: the library default is 10000ms, which alone equals the watchdog window.
  clientPost.setConnectionTimeout(1500);
  if (!clientPost.connect(serverAddress, _tcpPort))
  {
    Serial.println("Connection to API failed");
    clientPost.stop(); // free the socket; NativeEthernet does not close it on destruction
    _watchdog.feed();
    return false;
  }

  _watchdog.feed();

  clientPost.println(httpMethod + " " + endPoint + " HTTP/1.1");
  clientPost.print("Host: ");
  clientPost.println(serverAddress);
  clientPost.println("Content-Type: application/json");
  clientPost.print("Content-Length: ");
  clientPost.println(json.length());
  clientPost.println("Connection: close");
  clientPost.println();
  clientPost.println(json);

  // Wait for the response, but hard-bounded and feeding the watchdog while we
  // wait, so a connected-but-silent API can never stall the board.
  const unsigned long responseTimeoutMs = 3000;
  unsigned long start = millis();
  while (clientPost.connected() && clientPost.available() == 0)
  {
    if (millis() - start > responseTimeoutMs)
    {
      Serial.println("Timeout waiting for API response");
      clientPost.stop();
      _watchdog.feed();
      return false;
    }
    _watchdog.feed();
    delay(1);
  }

  // Read the status line, e.g. "HTTP/1.1 200 OK".
  clientPost.setTimeout(500);
  String statusLine = clientPost.readStringUntil('\n');
  clientPost.stop(); // free the socket so repeated requests cannot exhaust them
  _watchdog.feed();

  // The status code is the token between the first and second space.
  int firstSpace = statusLine.indexOf(' ');
  int statusCode = -1;
  if (firstSpace >= 0)
  {
    statusCode = statusLine.substring(firstSpace + 1, firstSpace + 4).toInt();
  }

  Serial.println("API responded with status: " + String(statusCode));
  return statusCode >= 200 && statusCode < 300;
}

void NetworkController::init(WDT_T4<WDT1> &watchdog, SDCard &sdCard, Laser &laser)
{
  _watchdog = watchdog;
  _sdCard = sdCard;
  byte mac[6];
  teensyMAC(mac);

  // NativeEthernet initialises the Teensy PHY inside Ethernet.begin(), so the
  // link status is only meaningful after the first begin() call. begin() also
  // resolves DHCP synchronously: if no lease is acquired within the timeout,
  // localIP() stays 0.0.0.0. Just after boot (or right after the cable is
  // plugged in) the link often needs a moment to negotiate, so the first
  // attempt can fail while later ones succeed. Retry until we get a valid IP.
  const int maxDhcpAttempts = 6;
  for (int attempt = 1; attempt <= maxDhcpAttempts && Ethernet.localIP() == IPAddress(0, 0, 0, 0); attempt++)
  {
    Serial.println("Requesting IP via DHCP (attempt " + String(attempt) + ")");
    _watchdog.feed();
    Ethernet.begin(mac, 5000, 2000);
    _watchdog.feed();

    if (Ethernet.localIP() == IPAddress(0, 0, 0, 0))
    {
      delay(500);
    }
  }

  if (Ethernet.linkStatus() == LinkOFF)
  {
    Serial.println("No network cable connected");
    _connectionStatus = ConnectionStatus::NoNetworkCableConnected;
    _watchdog.feed();
    return;
  }

  if (Ethernet.localIP() == IPAddress(0, 0, 0, 0))
  {
    Serial.println("DHCP failed, no IP assigned (link is up)");
    _connectionStatus = ConnectionStatus::NoNetworkCableConnected;
    _watchdog.feed();
    return;
  }

  Serial.println("Assigned DHCP IP: " + ipToString(Ethernet.localIP()));

  _udpClient.begin(_udpPort);
  _server.begin();
  _watchdog.feed();
}

String NetworkController::onAdoptionRequest(IPAddress &serverAddress, const String &json)
{
  _watchdog.feed();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return "{\"success\":false,\"error\":\"invalid json\"}";
  }

  // Confirm the adoption with the controller first; only persist the settings if
  // the controller acknowledges with a 2xx status. Otherwise we would store a
  // half-completed adoption the controller does not know about.
  bool acknowledged = sendNetworkRequest("POST", "/laserconnection/connect", serverAddress, json);
  if (!acknowledged)
  {
    Serial.println("Controller did not acknowledge adoption, settings not saved");
    return "{\"success\":false,\"error\":\"controller did not acknowledge\"}";
  }

  settingsModel settings = Settings::getSettings();

  strncpy(settings.uuid, doc["Uuid"] | "", sizeof(settings.uuid));
  settings.uuid[sizeof(settings.uuid) - 1] = '\0';

  strncpy(settings.name, doc["Name"] | "", sizeof(settings.name));
  settings.name[sizeof(settings.name) - 1] = '\0';

  settings.modelType = doc["ModelType"] | 0;
  settings.connectionStatus = doc["Status"] | 0;

  // The controller IP is the source IP of the adoption request itself, not a
  // field in the JSON body (doc["IPAddress"] would be the laser's own broadcast
  // IP). serverAddress is the remoteIp of the PC/API that sent the /adopt call.
  for (int i = 0; i < 4; i++)
  {
    settings.controllerIp[i] = serverAddress[i];
  }
  Serial.println("Adoption request from controller with IP: " + ipToString(serverAddress));

  Serial.println("Saving settings");
  Serial.println("Controller IP: " + String(settings.controllerIp[0]) + "." + String(settings.controllerIp[1]) + "." + String(settings.controllerIp[2]) + "." + String(settings.controllerIp[3]));
  bool settingsStored = Settings::setSettings(settings);
  if (!settingsStored)
  {
    Serial.println("Settings not stored");
  }

  Settings::saveSettings();
  sendSettingsToController(settings);
  return "{\"success\":true}";
}

String NetworkController::onSettingsUpdate(IPAddress &serverAddress, const String &json)
{
  _watchdog.feed();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return "{\"success\":false,\"error\":\"invalid json\"}";
  }

  settingsModel settings = Settings::getSettings();
  strlcpy(settings.name, doc["Name"] | "", sizeof(settings.name));
  settings.maxPowerPerlaserInPercentage = doc["MaxPowerPerlaserInPercentage"].as<int>();
  settings.projectionTopInPercentage = doc["ProjectionTopInPercentage"].as<int>();
  settings.projectionBottomInPercentage = doc["ProjectionBottomInPercentage"].as<int>();
  settings.projectionLeftInPercentage = doc["ProjectionLeftInPercentage"].as<int>();
  settings.projectionRightInPercentage = doc["ProjectionRightInPercentage"].as<int>();

  Serial.println("Saving updated settings");
  bool settingsStored = Settings::setSettings(settings);
  if (!settingsStored)
  {
    Serial.println("Settings not stored");
  }

  Settings::saveSettings();
  sendSettingsToController(settings);
  return "{\"success\":true}";
}

/**
  @brief Handle a GET request asking for the laser's current settings and serialize
         them back to the caller as the response body.

  @return the current settings serialized as JSON
 */
String NetworkController::onSettingsRequest(IPAddress &serverAddress, const String &json)
{
  _watchdog.feed();

  settingsModel settings = Settings::getSettings();
  JsonDocument doc;
  setDoc(doc, settings);

  String response;
  serializeJson(doc, response);
  return response;
}

String NetworkController::onIncommingAliveCheck(IPAddress &serverAddress, const String &json)
{
  _watchdog.feed();
  Serial.println("Incomming alive check");
  return "{\"success\":true}";
}

String NetworkController::onSDCardFilesRequest(IPAddress &serverAddress, const String &json)
{
  _watchdog.feed();

  std::vector<String> files = _sdCard.getLzsFiles();
  JsonDocument doc;
  JsonArray fileArray = doc.to<JsonArray>();
  for (const String &file : files)
  {
    JsonObject model = fileArray.add<JsonObject>();
    model["filename"] = file;

    // Size in whole KB, rounded up so a non-empty file never shows as 0 KB.
    uint32_t sizeKb = 0;
    File entry = SD.open(file.c_str());
    if (entry)
    {
      sizeKb = (uint32_t)((entry.size() + 1023) / 1024);
      entry.close();
    }
    model["fileSizeKb"] = sizeKb;
  }

  String response;
  serializeJson(doc, response);
  return response;
}

String NetworkController::onSDCardBinaryUpload(IPAddress &serverAddress, const String &json)
{
  // The body was already streamed to the SD card in getRequestData; report the
  // outcome it recorded rather than touching the (unused) json body.
  return _lastBinaryUploadSucceeded
             ? "{\"success\":true}"
             : "{\"success\":false}";
}

String NetworkController::onLiveShowUpload(IPAddress &serverAddress, const String &json)
{
  // The body was already streamed into RAM in getRequestData; report the outcome
  // it recorded. On success it also set CurrentLaserMode = Network, so
  // NetworkPlayMode will pick up the blob on the next loop iteration.
  return _lastBinaryUploadSucceeded
             ? "{\"success\":true}"
             : "{\"success\":false}";
}

String NetworkController::onPlaySDCardFile(IPAddress &serverAddress, const String &json)
{
  _watchdog.feed();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return "{\"success\":false,\"error\":\"invalid json\"}";
  }

  String filename = doc["filename"];
  if (filename.length() == 0)
  {
    return "{\"success\":false,\"error\":\"missing filename\"}";
  }
  if (!SD.exists(filename.c_str()))
  {
    return "{\"success\":false,\"error\":\"file not found\"}";
  }

  // Hand off to PlaySDFileMode, which opens/validates/streams the .lzs from SD.
  // The show streams frame-by-frame off the SD card, so size is not limited by
  // RAM (unlike a live pattern/animation upload).
  SelectedSDCardFilename = filename;
  CurrentLaserMode = LaserMode::SDCardMode;

  return "{\"success\":true}";
}

String NetworkController::onStopPlayback(IPAddress &serverAddress, const String &json)
{
  // Return to no mode; the main loop then calls the active mode's stop(), which
  // blanks the laser and releases its source. Works for both an SD lasershow and
  // a live pattern/animation preview.
  CurrentLaserMode = LaserMode::NotSelected;
  return "{\"success\":true}";
}

String NetworkController::onPlaybackStatusRequest(IPAddress &serverAddress, const String &json)
{
  // Position is maintained by the playing mode (frame granularity); the
  // frontend polls this to move its timeline slider.
  bool playing = CurrentLaserMode == LaserMode::SDCardMode ||
                 CurrentLaserMode == LaserMode::Network;

  JsonDocument doc;
  doc["playing"] = playing;
  doc["positionMs"] = (uint32_t)PlaybackPositionMs;

  String response;
  serializeJson(doc, response);
  return response;
}

String NetworkController::onSeekPlayback(IPAddress &serverAddress, const String &json)
{
  _watchdog.feed();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return "{\"success\":false,\"error\":\"invalid json\"}";
  }

  long positionMs = doc["positionMs"] | -1L;
  if (positionMs < 0)
  {
    return "{\"success\":false,\"error\":\"missing positionMs\"}";
  }

  // Consumed by the playing mode on its next produce iteration; ignored when
  // nothing is playing.
  SeekRequestMs = (int32_t)positionMs;
  return "{\"success\":true}";
}

String NetworkController::onSDCardDeleteFile(IPAddress &serverAddress, const String &json)
{
  _watchdog.feed();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return "{\"success\":false,\"error\":\"invalid json\"}";
  }

  String filename = doc["filename"];
  bool success = _sdCard.deleteLzsFile(filename);
  Serial.println("SD card delete success: " + String(success) + " for file: " + filename);

  return success
             ? "{\"success\":true}"
             : "{\"success\":false}";
}

void NetworkController::onApiCall(String type, String endPoint, IPAddress serverAddress, CallbackFunc cb)
{
  String json = "";
  cb(serverAddress, json);
}

void NetworkController::getRequestData(EthernetClient &client, String &httpMethod, String &endPoint, String &json)
{
  const unsigned long timeoutMs = 2000;

  // Blocking single-byte read that waits (up to timeoutMs) for data instead of
  // giving up the moment the socket buffer is momentarily empty. Returns -1 on
  // timeout or disconnect. Feeds the watchdog while waiting so a slow request
  // does not trip a reset.
  auto readByte = [&]() -> int
  {
    unsigned long start = millis();
    while (client.connected() || client.available())
    {
      if (client.available())
      {
        return client.read();
      }
      if (millis() - start > timeoutMs)
      {
        break;
      }
      _watchdog.feed();
    }
    return -1;
  };

  // --- Read headers until the blank line that terminates them ---
  String header = "";
  int c;
  while ((c = readByte()) >= 0)
  {
    header += (char)c;
    if (header.endsWith("\r\n\r\n"))
    {
      break;
    }
  }
  if (header.length() == 0)
  {
    return; // nothing arrived
  }

  // --- Parse the request line: "<METHOD> /<endpoint> HTTP/1.1" ---
  int indexOfEndpointStart = header.indexOf(" /");
  int indexOfEndpointEnd = header.indexOf(' ', indexOfEndpointStart + 1);
  endPoint = header.substring(indexOfEndpointStart + 1, indexOfEndpointEnd);
  httpMethod = header.substring(0, indexOfEndpointStart);

  // --- Determine how the body is framed (header names are case-insensitive) ---
  String lowerHeader = header;
  lowerHeader.toLowerCase();

  bool chunked = lowerHeader.indexOf("transfer-encoding:") >= 0 &&
                 lowerHeader.indexOf("chunked") >= 0;

  int contentLength = 0;
  int clIdx = lowerHeader.indexOf("content-length:");
  if (clIdx >= 0)
  {
    contentLength = header.substring(clIdx + 15).toInt();
  }

  // --- Streaming binary upload: write the body straight to the SD card ---
  // The .lzs show can be many KB; accumulating it in a String (like the JSON
  // path below) is what caused the ArduinoJson NoMemory. Instead we stream it
  // into a file through a small fixed buffer, so RAM use is O(buffer) not
  // O(file). The target filename arrives in the X-Filename header; the API
  // sends a fixed Content-Length (never chunked) for this endpoint.
  if (httpMethod == "POST" && endPoint == "/sd-card-binary")
  {
    _lastBinaryUploadSucceeded = false;

    String filename = "";
    int nameIdx = lowerHeader.indexOf("x-filename:");
    if (nameIdx >= 0)
    {
      int valueStart = nameIdx + 11; // strlen("x-filename:")
      int lineEnd = header.indexOf("\r\n", valueStart);
      if (lineEnd < 0)
      {
        lineEnd = header.length();
      }
      filename = header.substring(valueStart, lineEnd);
      filename.trim();
    }

    // Only bare filenames are accepted: no path separators or "..", so an
    // uploaded name can never point outside the SD card root.
    bool filenameSafe = filename.length() > 0 &&
                        filename.indexOf('/') < 0 &&
                        filename.indexOf('\\') < 0 &&
                        filename.indexOf("..") < 0;

    if (chunked || !filenameSafe || contentLength <= 0)
    {
      return; // unsupported framing or bad target -> reported as failure
    }

    File out = _sdCard.openForWrite(filename);
    if (!out)
    {
      return;
    }

    uint8_t buffer[512];
    int bufferLength = 0;
    int bytesWritten = 0;
    for (int i = 0; i < contentLength; i++)
    {
      int b = readByte();
      if (b < 0)
      {
        break; // client stalled or disconnected
      }

      buffer[bufferLength++] = (uint8_t)b;
      if (bufferLength == (int)sizeof(buffer))
      {
        out.write(buffer, bufferLength);
        bytesWritten += bufferLength;
        bufferLength = 0;
      }
    }
    if (bufferLength > 0)
    {
      out.write(buffer, bufferLength);
      bytesWritten += bufferLength;
    }
    out.close();

    _lastBinaryUploadSucceeded = (bytesWritten == contentLength);
    return;
  }

  // --- Streaming binary LIVE upload: stream the body into a RAM buffer ---
  // Patterns/animations are played live and NOT stored on the laser, so the body
  // (an ".lzs" blob) goes straight into RAM (LiveShowData) rather than the SD
  // card. A size guard rejects anything too large to protect controller RAM. On
  // success NetworkPlayMode picks up the blob and begins looping it. The
  // connection is closed per request (Connection: close), so an unread body from
  // a rejected upload is simply discarded -- no need to drain it.
  if (httpMethod == "POST" && endPoint == "/live-binary")
  {
    _lastBinaryUploadSucceeded = false;

    if (chunked || contentLength <= 0 || (uint32_t)contentLength > MAX_LIVE_SHOW_BYTES)
    {
      return; // unsupported framing or oversized -> reported as failure
    }

    LiveShowData.clear();
    LiveShowData.reserve((size_t)contentLength);

    int bytesRead = 0;
    for (int i = 0; i < contentLength; i++)
    {
      int b = readByte();
      if (b < 0)
      {
        break; // client stalled or disconnected
      }
      LiveShowData.push_back((uint8_t)b);
      bytesRead++;
    }

    if (bytesRead == contentLength)
    {
      LiveShowPending = true;
      CurrentLaserMode = LaserMode::Network;
      _lastBinaryUploadSucceeded = true;
    }
    else
    {
      LiveShowData.clear();
    }
    return;
  }

  // --- Read the body ---
  if (chunked)
  {
    // Chunked encoding: each chunk is "<hex size>\r\n<data>\r\n", terminated by
    // a zero-size chunk. Decode it back into the raw JSON payload. This is what
    // .NET's PutAsJsonAsync sends when it omits Content-Length.
    while (true)
    {
      String sizeLine = "";
      while ((c = readByte()) >= 0)
      {
        if (c == '\n')
        {
          break;
        }
        if (c != '\r')
        {
          sizeLine += (char)c;
        }
      }

      int chunkSize = (int)strtol(sizeLine.c_str(), nullptr, 16);
      if (chunkSize <= 0)
      {
        break; // final (zero-size) chunk
      }

      for (int i = 0; i < chunkSize; i++)
      {
        c = readByte();
        if (c < 0)
        {
          return;
        }
        json += (char)c;
      }

      // Consume the trailing \r\n that follows the chunk data.
      readByte();
      readByte();
    }
  }
  else
  {
    // Fixed-length body: read exactly Content-Length bytes, tolerating the
    // payload arriving in multiple TCP segments.
    for (int i = 0; i < contentLength; i++)
    {
      c = readByte();
      if (c < 0)
      {
        break;
      }
      json += (char)c;
    }
  }
}

std::vector<KeyValue> NetworkController::createDict()
{
  return {
      {"GET", "/alive", [this](IPAddress serverAddress, const String json)
       {
         return onIncommingAliveCheck(serverAddress, json);
       }},
      {"GET", "/settings", [this](IPAddress serverAddress, const String json)
       {
         return onSettingsRequest(serverAddress, json);
       }},
      {"POST", "/adopt", [this](IPAddress serverAddress, const String json)
       {
         return onAdoptionRequest(serverAddress, json);
       }},
      {"PUT", "/settings", [this](IPAddress serverAddress, const String json)
       {
         return onSettingsUpdate(serverAddress, json);
       }},
      {"GET", "/sd-card", [this](IPAddress serverAddress, const String json)
       {
         return onSDCardFilesRequest(serverAddress, json);
       }},
      {"PUT", "/sd-card", [this](IPAddress serverAddress, const String json)
       {
         return onSDCardDeleteFile(serverAddress, json);
       }},
      {"POST", "/sd-card-binary", [this](IPAddress serverAddress, const String json)
       {
         return onSDCardBinaryUpload(serverAddress, json);
       }},
      {"POST", "/live-binary", [this](IPAddress serverAddress, const String json)
       {
         return onLiveShowUpload(serverAddress, json);
       }},
      {"POST", "/sd-card-play", [this](IPAddress serverAddress, const String json)
       {
         return onPlaySDCardFile(serverAddress, json);
       }},
      {"POST", "/stop", [this](IPAddress serverAddress, const String json)
       {
         return onStopPlayback(serverAddress, json);
       }},
      {"GET", "/playback-status", [this](IPAddress serverAddress, const String json)
       {
         return onPlaybackStatusRequest(serverAddress, json);
       }},
      {"POST", "/seek", [this](IPAddress serverAddress, const String json)
       {
         return onSeekPlayback(serverAddress, json);
       }},
  };
}

String NetworkController::executeCallback(const String &httpMethod, const String &endPoint, IPAddress serverAddress, const String json)
{
  static auto dict = createDict();
  for (auto &entry : dict)
  {
    if (entry.httpMethod == httpMethod && entry.endPoint == endPoint)
    {
      Serial.println("Executing incomming API call: method: " + httpMethod + " endpoint: " + endPoint + " serverAddress: " + ipToString(serverAddress));
      return entry.cb(serverAddress, json);
    }
  }
  Serial.println("Geen match gevonden!");
  return "";
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

    // Dispatch regardless of whether a body was sent: GET requests that ask for
    // data have no body but still need to reach their handler. The handler
    // returns the JSON body to send back; an empty body (no matching endpoint)
    // falls back to the default {"success":true} so callers always get valid JSON.
    String responseBody = executeCallback(httpMethod, endPoint, remoteIp, json);
    if (responseBody.length() == 0)
    {
      responseBody = "{\"success\":true}";
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(responseBody.length());
    client.println("Connection: close");
    client.println();
    client.print(responseBody);
    client.stop();
  }
}

void NetworkController::sendBroadcast()
{
  settingsModel settings = Settings::getSettings();

  JsonDocument doc;
  setDoc(doc, settings);

  String jsonString;
  serializeJson(doc, jsonString);

  // Use the limited broadcast address (255.255.255.255). The FNET stack used by
  // NativeEthernet often fails to send to a subnet-directed broadcast (e.g.
  // 192.168.1.255) because it needs to ARP for it, silently dropping the packet.
  IPAddress broadcastIp(255, 255, 255, 255);

  if (!_udpClient.beginPacket(broadcastIp, _udpPort))
  {
    Serial.println("UDP beginPacket failed (broadcast)");
    _watchdog.feed();
    return;
  }

  _udpClient.write((const uint8_t *)jsonString.c_str(), jsonString.length());

  if (!_udpClient.endPacket())
  {
    Serial.println("UDP endPacket failed (broadcast)");
  }
  else
  {
    Serial.println("Broadcast send: " + jsonString);
  }

  _watchdog.feed();
}

void NetworkController::disconnect()
{
  EthernetClient client = _server.available();
  client.stop();
}

String NetworkController::getAssignedIP()
{
  Serial.println(ipToString(Ethernet.localIP()));
  return ipToString(Ethernet.localIP());
}

ConnectionStatus NetworkController::getConnectionStatus()
{
  return _connectionStatus;
}