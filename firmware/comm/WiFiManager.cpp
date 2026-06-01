#include "WiFiManager.h"
#include "../config.h"
#include "../core/Security.h"
#include "EmailManager.h"
#include <AsyncJson.h>
#include <ArduinoJson.h>

DNSServer  WiFiManager::_dns;
bool       WiFiManager::_apActive = false;
APPhase    WiFiManager::_phase    = APPhase::REGISTRATION;

void WiFiManager::begin() { WiFi.mode(WIFI_OFF); }

void WiFiManager::startAP(APPhase phase) {
  _phase = phase;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SETUP_AP_SSID, SETUP_AP_PASS, SETUP_AP_CHANNEL);
  delay(200);
  // DNS: redirect everything to 192.168.4.1
  _dns.start(53, "*", WiFi.softAPIP());
  _apActive = true;
  Serial.printf("[WiFi] AP started: %s  IP: %s\n",
    SETUP_AP_SSID, WiFi.softAPIP().toString().c_str());
}

void WiFiManager::stopAP() {
  _dns.stop();
  WiFi.softAPdisconnect(true);
  _apActive = false;
}

bool WiFiManager::isAPActive() { return _apActive; }

void WiFiManager::processAP() {
  if (_apActive) _dns.processNextRequest();
}

bool WiFiManager::connectSTA(const String& ssid, const String& pass,
                               uint32_t timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > timeoutMs) return false;
    delay(200);
  }
  return true;
}

bool WiFiManager::isConnected() { return WiFi.status() == WL_CONNECTED; }

void WiFiManager::setupRoutes(AsyncWebServer& srv, APPhase phase) {
  // Captive portal redirect
  srv.onNotFound([](AsyncWebServerRequest* req) {
    req->redirect("http://192.168.4.1/");
  });

  // Serve index pages
  srv.on("/", HTTP_GET, [phase](AsyncWebServerRequest* req) {
    String path = (phase == APPhase::REGISTRATION)
      ? PATH_WEB_INDEX : PATH_WEB_WIFI;
    req->redirect(path);
  });
  srv.serveStatic("/web/", LittleFS, "/web/");

  if (phase == APPhase::REGISTRATION) {
    // POST /api/register
    srv.addHandler(new AsyncCallbackJsonWebHandler("/api/register",
      [](AsyncWebServerRequest* req, JsonVariant& body) {
        WiFiManager::_handleRegister(req, body);
      }));
    // POST /api/restore (multipart)
    srv.on("/api/restore", HTTP_POST,
      [](AsyncWebServerRequest* req) { WiFiManager::_handleRestore(req); });
    // GET /api/status
    srv.on("/api/status", HTTP_GET,
      [](AsyncWebServerRequest* req) { WiFiManager::_handleStatus(req); });
  } else {
    // POST /api/test-email
    srv.addHandler(new AsyncCallbackJsonWebHandler("/api/test-email",
      [](AsyncWebServerRequest* req, JsonVariant& body) {
        WiFiManager::_handleTestEmail(req, body);
      }));
    // POST /api/save-network
    srv.addHandler(new AsyncCallbackJsonWebHandler("/api/save-network",
      [](AsyncWebServerRequest* req, JsonVariant& body) {
        WiFiManager::_handleSaveNetwork(req, body);
      }));
  }
}

void WiFiManager::_handleRegister(AsyncWebServerRequest* req, JsonVariant& body) {
  String email    = body["email"]    | "";
  String password = body["password"] | "";
  String name     = body["name"]     | "";
  String dob      = body["dob"]      | "";
  String address  = body["address"]  | "";
  String phone    = body["phone"]    | "";
  String company  = body["company"]  | "";

  if (email.isEmpty() || password.length() < 8 || name.isEmpty()) {
    req->send(400, "application/json", "{\"success\":false,\"message\":\"Missing required fields\"}");
    return;
  }
  UserProfile p;
  p.email        = email;
  p.passwordHash = Security::hashPIN(password.c_str(), password.length());
  p.name = name; p.dob = dob; p.address = address;
  p.phone = phone; p.company = company;

  if (!Storage::saveProfile(p)) {
    req->send(500, "application/json", "{\"success\":false,\"message\":\"Storage error\"}");
    return;
  }
  req->send(200, "application/json",
    "{\"success\":true,\"message\":\"Account created. Continue on device.\"}");
}

void WiFiManager::_handleRestore(AsyncWebServerRequest* req) {
  // Simplified: full implementation handles multipart file upload
  req->send(200, "application/json",
    "{\"success\":true,\"vaultRestored\":false}");
}

void WiFiManager::_handleStatus(AsyncWebServerRequest* req) {
  DynamicJsonDocument doc(128);
  doc["phase"]      = (int)_phase;
  doc["first_boot"] = Storage::isFirstBoot();
  String out; serializeJson(doc, out);
  req->send(200, "application/json", out);
}

void WiFiManager::_handleTestEmail(AsyncWebServerRequest* req, JsonVariant& body) {
  NetworkConfig cfg;
  cfg.smtp_server = body["smtp_server"] | "";
  cfg.smtp_port   = body["smtp_port"]   | 587;
  cfg.smtp_email  = body["smtp_email"]  | "";
  cfg.smtp_pass   = body["smtp_pass"]   | "";
  cfg.to_user     = body["to_user"]     | "";
  cfg.to_admin    = body["to_admin"]    | "";

  bool ok = EmailManager::testConnection(cfg);
  if (ok) req->send(200, "application/json",
    "{\"success\":true,\"message\":\"Test email sent\"}");
  else    req->send(200, "application/json",
    "{\"success\":false,\"message\":\"SMTP connection failed\"}");
}

void WiFiManager::_handleSaveNetwork(AsyncWebServerRequest* req, JsonVariant& body) {
  NetworkConfig cfg;
  cfg.wifi_ssid   = body["wifi_ssid"]   | "";
  cfg.wifi_pass   = body["wifi_pass"]   | "";
  cfg.smtp_server = body["smtp_server"] | "";
  cfg.smtp_port   = body["smtp_port"]   | 587;
  cfg.smtp_email  = body["smtp_email"]  | "";
  cfg.smtp_pass   = body["smtp_pass"]   | "";
  cfg.to_user     = body["to_user"]     | "";
  cfg.to_admin    = body["to_admin"]    | "";

  if (!Storage::saveNetwork(cfg)) {
    req->send(500, "application/json", "{\"success\":false,\"message\":\"Storage error\"}");
    return;
  }
  req->send(200, "application/json", "{\"success\":true}");
}

// ── CORRECTED _handleRegister ─────────────────────────────────
// Now also receives company (optional) and admin_email from the
// updated index.html registration form.
// Replace the original _handleRegister body with this version:
/*
void WiFiManager::_handleRegister_v2(AsyncWebServerRequest* req, JsonVariant& body) {
  String email      = body["email"]        | "";
  String password   = body["password"]     | "";
  String name       = body["name"]         | "";
  String dob        = body["dob"]          | "";
  String phone      = body["phone"]        | "";
  String company    = body["company"]      | "";     // optional
  String adminEmail = body["admin_email"]  | "";

  // Validate required fields
  if (email.isEmpty() || password.length() < 8 || name.isEmpty()
      || dob.isEmpty() || phone.isEmpty() || adminEmail.isEmpty()) {
    req->send(400, "application/json",
      "{\"success\":false,\"message\":\"Missing required fields\"}");
    return;
  }
  // Basic admin email format check
  if (adminEmail.indexOf('@') < 0) {
    req->send(400, "application/json",
      "{\"success\":false,\"message\":\"Invalid admin email\"}");
    return;
  }

  // Save profile
  UserProfile p;
  p.email        = email;
  p.passwordHash = Security::hashPIN(password.c_str(), password.length());
  p.name         = name;
  p.dob          = dob;
  p.phone        = phone;
  p.company      = company;       // may be empty
  p.admin_email  = adminEmail;
  if (!Storage::saveProfile(p)) {
    req->send(500, "application/json", "{\"success\":false,\"message\":\"Storage error\"}");
    return;
  }
  // Pre-populate admin email in network config
  NetworkConfig net;
  Storage::loadNetwork(net);
  net.to_admin = adminEmail;
  Storage::saveNetwork(net);

  req->send(200, "application/json",
    "{\"success\":true,\"message\":\"Account created. Continue on device.\"}");
}
*/

// ── GET /api/config ─────────────────────────────────────────
// Returns saved config values that the wifi_setup.html page
// can use to pre-fill fields (admin email from registration).
// Add to setupRoutes() for APPhase::NETWORK_SETUP:
/*
  srv.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* req) {
    DynamicJsonDocument doc(256);
    NetworkConfig net;
    if (Storage::loadNetwork(net)) {
      doc["admin_email"] = net.to_admin;
    }
    String out; serializeJson(doc, out);
    req->send(200, "application/json", out);
  });
*/
