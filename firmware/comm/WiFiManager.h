#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "../core/Storage.h"

// =============================================================
//  WiFiManager.h | Captive portal AP + STA connection
//  Phase 1 AP: registration / restore
//  Phase 2 AP: WiFi + email configuration
// =============================================================
enum class APPhase : uint8_t { REGISTRATION = 1, NETWORK_SETUP = 2 };

class WiFiManager {
public:
  static void begin();

  // AP management
  static void startAP(APPhase phase);
  static void stopAP();
  static bool isAPActive();
  static void processAP();   // call in loop() while AP active

  // STA (home network) connection
  static bool connectSTA(const String& ssid, const String& pass,
                          uint32_t timeoutMs = 15000);
  static bool isConnected();

  // API handlers registered on AsyncWebServer
  static void setupRoutes(AsyncWebServer& srv, APPhase phase);

private:
  static DNSServer     _dns;
  static bool          _apActive;
  static APPhase       _phase;

  static void _handleRegister(AsyncWebServerRequest* req, JsonVariant& body);
  static void _handleRestore (AsyncWebServerRequest* req);
  static void _handleStatus  (AsyncWebServerRequest* req);
  static void _handleTestEmail(AsyncWebServerRequest* req, JsonVariant& body);
  static void _handleSaveNetwork(AsyncWebServerRequest* req, JsonVariant& body);
};
