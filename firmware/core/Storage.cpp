#include "Storage.h"
#include "Security.h"
#include "../config.h"

bool Storage::begin() {
  if (!LittleFS.begin(true)) {   // true = format on failure
    Serial.println("[FS] LittleFS mount failed");
    return false;
  }
  // Ensure /web directory exists
  if (!LittleFS.exists("/web")) LittleFS.mkdir("/web");
  return true;
}

bool Storage::isFirstBoot() {
  return !LittleFS.exists(PATH_CONFIG);
}

// ── Atomic write ──────────────────────────────────────────────
bool Storage::_atomicWrite(const String& path, const String& json) {
  String tmp = path + ".tmp";
  File f = LittleFS.open(tmp, "w");
  if (!f) return false;
  f.print(json);
  f.close();
  LittleFS.remove(path);
  return LittleFS.rename(tmp, path);
}

String Storage::_readFile(const String& path) {
  if (!LittleFS.exists(path)) return "";
  File f = LittleFS.open(path, "r");
  if (!f) return "";
  String s = f.readString();
  f.close();
  return s;
}

// ── Salt ──────────────────────────────────────────────────────
bool Storage::saveSalt(const uint8_t* salt, size_t len) {
  String b64 = base64::encode(salt, len);
  DynamicJsonDocument doc(256);
  // Load existing config if present to preserve other fields
  String existing = _readFile(PATH_CONFIG);
  if (existing.length() > 0) deserializeJson(doc, existing);
  doc["salt"] = b64;
  String out; serializeJson(doc, out);
  return _atomicWrite(PATH_CONFIG, out);
}

bool Storage::loadSalt(uint8_t* salt, size_t len) {
  String raw = _readFile(PATH_CONFIG);
  if (raw.isEmpty()) return false;
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, raw) != DeserializationError::Ok) return false;
  if (!doc.containsKey("salt")) return false;
  String b64 = doc["salt"].as<String>();
  String dec = base64::decode(b64);
  if (dec.length() != len) return false;
  memcpy(salt, dec.c_str(), len);
  return true;
}

// ── Device config ─────────────────────────────────────────────
bool Storage::saveConfig(const DeviceConfig& c) {
  DynamicJsonDocument doc(1024);
  // Preserve salt
  String existing = _readFile(PATH_CONFIG);
  if (!existing.isEmpty()) {
    DynamicJsonDocument tmp(1024);
    deserializeJson(tmp, existing);
    if (tmp.containsKey("salt")) doc["salt"] = tmp["salt"];
  }
  doc["language"]          = c.language;
  doc["autolock_ms"]       = c.autolock_ms;
  doc["backup_freq_days"]  = c.backup_freq_days;
  doc["hid_ble"]           = c.hid_ble;
  doc["pin_hash"]          = c.pinHash;
  doc["fp_slot_count"]     = c.fpSlotCount;
  doc["setup_done"]        = c.setupDone;
  doc["wifi_setup_done"]   = c.wifiSetupDone;
  JsonArray slots = doc.createNestedArray("fp_slots");
  for (uint8_t i = 0; i < FP_MAX_SLOTS; i++) slots.add(c.fpSlots[i]);
  String out; serializeJson(doc, out);
  return _atomicWrite(PATH_CONFIG, out);
}

bool Storage::loadConfig(DeviceConfig& c) {
  String raw = _readFile(PATH_CONFIG);
  if (raw.isEmpty()) return false;
  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, raw) != DeserializationError::Ok) return false;
  c.language          = doc["language"]         | 0;
  c.autolock_ms       = doc["autolock_ms"]      | (uint32_t)AUTOLOCK_DEFAULT_MS;
  c.backup_freq_days  = doc["backup_freq_days"] | 7;
  c.hid_ble           = doc["hid_ble"]          | false;
  c.pinHash           = doc["pin_hash"]         | "";
  c.fpSlotCount       = doc["fp_slot_count"]    | 0;
  c.setupDone         = doc["setup_done"]       | false;
  c.wifiSetupDone     = doc["wifi_setup_done"]  | false;
  JsonArray slots = doc["fp_slots"];
  for (uint8_t i = 0; i < FP_MAX_SLOTS && i < slots.size(); i++)
    c.fpSlots[i] = slots[i];
  return true;
}

// ── User profile ──────────────────────────────────────────────
bool Storage::saveProfile(const UserProfile& p) {
  DynamicJsonDocument doc(512);
  doc["email"]         = p.email;
  doc["password_hash"] = p.passwordHash;
  doc["name"]          = Security::encryptStr(p.name);
  doc["dob"]           = Security::encryptStr(p.dob);
  doc["address"]       = Security::encryptStr(p.address);
  doc["phone"]         = Security::encryptStr(p.phone);
  doc["company"]       = Security::encryptStr(p.company);
  String out; serializeJson(doc, out);
  return _atomicWrite(PATH_PROFILE, out);
}

bool Storage::loadProfile(UserProfile& p) {
  String raw = _readFile(PATH_PROFILE);
  if (raw.isEmpty()) return false;
  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, raw) != DeserializationError::Ok) return false;
  p.email         = doc["email"]         | "";
  p.passwordHash  = doc["password_hash"] | "";
  p.name          = Security::decryptStr(doc["name"]    | "");
  p.dob           = Security::decryptStr(doc["dob"]     | "");
  p.address       = Security::decryptStr(doc["address"] | "");
  p.phone         = Security::decryptStr(doc["phone"]   | "");
  p.company       = Security::decryptStr(doc["company"] | "");
  return true;
}

// ── Network config ────────────────────────────────────────────
bool Storage::saveNetwork(const NetworkConfig& n) {
  DynamicJsonDocument doc(512);
  doc["wifi_ssid"]    = Security::encryptStr(n.wifi_ssid);
  doc["wifi_pass"]    = Security::encryptStr(n.wifi_pass);
  doc["smtp_server"]  = n.smtp_server;
  doc["smtp_port"]    = n.smtp_port;
  doc["smtp_email"]   = n.smtp_email;
  doc["smtp_pass"]    = Security::encryptStr(n.smtp_pass);
  doc["to_user"]      = n.to_user;
  doc["to_admin"]     = n.to_admin;
  String out; serializeJson(doc, out);
  return _atomicWrite(PATH_NETWORK, out);
}

bool Storage::loadNetwork(NetworkConfig& n) {
  String raw = _readFile(PATH_NETWORK);
  if (raw.isEmpty()) return false;
  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, raw) != DeserializationError::Ok) return false;
  n.wifi_ssid   = Security::decryptStr(doc["wifi_ssid"]  | "");
  n.wifi_pass   = Security::decryptStr(doc["wifi_pass"]  | "");
  n.smtp_server = doc["smtp_server"]  | "";
  n.smtp_port   = doc["smtp_port"]    | 587;
  n.smtp_email  = doc["smtp_email"]   | "";
  n.smtp_pass   = Security::decryptStr(doc["smtp_pass"]  | "");
  n.to_user     = doc["to_user"]      | "";
  n.to_admin    = doc["to_admin"]     | "";
  return true;
}

// ── Vault (credentials) ───────────────────────────────────────
bool Storage::saveVault(const std::vector<Credential>& creds) {
  DynamicJsonDocument doc(16384);
  JsonArray arr = doc.createNestedArray("credentials");
  for (const auto& c : creds) {
    JsonObject obj = arr.createNestedObject();
    obj["name"]         = c.name;
    obj["username_enc"] = c.username_enc;
    obj["password_enc"] = c.password_enc;
  }
  String out; serializeJson(doc, out);
  return _atomicWrite(PATH_VAULT, out);
}

bool Storage::loadVault(std::vector<Credential>& creds) {
  String raw = _readFile(PATH_VAULT);
  if (raw.isEmpty()) return true;   // empty vault is OK
  DynamicJsonDocument doc(16384);
  if (deserializeJson(doc, raw) != DeserializationError::Ok) return false;
  creds.clear();
  for (JsonObject obj : doc["credentials"].as<JsonArray>()) {
    Credential c;
    c.name         = obj["name"]         | "";
    c.username_enc = obj["username_enc"] | "";
    c.password_enc = obj["password_enc"] | "";
    creds.push_back(c);
  }
  return true;
}

bool Storage::appendCredential(const Credential& c) {
  std::vector<Credential> creds;
  loadVault(creds);
  creds.push_back(c);
  return saveVault(creds);
}

bool Storage::readVaultBlob(uint8_t* buf, size_t& len, size_t maxLen) {
  String raw = _readFile(PATH_VAULT);
  if (raw.isEmpty()) return false;
  len = min((size_t)raw.length(), maxLen);
  memcpy(buf, raw.c_str(), len);
  return true;
}

bool Storage::serveFile(const String& path, String& content) {
  content = _readFile(path);
  return !content.isEmpty();
}

bool Storage::writeWebFile(const String& path, const String& content) {
  File f = LittleFS.open(path, "w");
  if (!f) return false;
  f.print(content);
  f.close();
  return true;
}
