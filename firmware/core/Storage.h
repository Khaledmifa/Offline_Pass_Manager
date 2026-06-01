#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// =============================================================
//  Storage.h | LittleFS vault + config persistence
//  All credential fields stored as base64(IV||ciphertext).
//  Writes are atomic: write to .tmp then rename.
// =============================================================

struct Credential {
  String name;
  String username;      // plaintext after decrypt (never in flash)
  String password;      // plaintext after decrypt (never in flash)
  // Encrypted blobs stored on disk:
  String username_enc;
  String password_enc;
};

struct UserProfile {
  String email;
  String passwordHash;   // PBKDF2 hash
  String name;
  String dob;
  String address;
  String phone;
  String company;
};

struct NetworkConfig {
  String wifi_ssid;
  String wifi_pass;
  String smtp_server;
  int    smtp_port    = 587;
  String smtp_email;
  String smtp_pass;
  String to_user;
  String to_admin;
};

struct DeviceConfig {
  uint8_t  language     = 0;        // Language enum index
  uint32_t autolock_ms  = AUTOLOCK_DEFAULT_MS;
  uint8_t  backup_freq_days = 7;
  bool     hid_ble      = false;
  String   pinHash;
  uint8_t  fpSlotCount  = 0;
  bool     fpSlots[FP_MAX_SLOTS] = {};
  bool     setupDone    = false;
  bool     wifiSetupDone = false;
};

class Storage {
public:
  static bool begin();
  static bool isFirstBoot();

  // Profile (user registration)
  static bool saveProfile(const UserProfile& p);
  static bool loadProfile(UserProfile& p);

  // Device config
  static bool saveConfig(const DeviceConfig& c);
  static bool loadConfig(DeviceConfig& c);

  // Network + email config (stored encrypted)
  static bool saveNetwork(const NetworkConfig& n);
  static bool loadNetwork(NetworkConfig& n);

  // Vault (credentials)
  static bool saveVault(const std::vector<Credential>& creds);
  static bool loadVault(std::vector<Credential>& creds);
  static bool appendCredential(const Credential& c);

  // Salt for PBKDF2
  static bool saveSalt(const uint8_t* salt, size_t len);
  static bool loadSalt(uint8_t* salt, size_t len);

  // Raw vault blob for email backup
  static bool readVaultBlob(uint8_t* buf, size_t& len, size_t maxLen);

  // Web assets
  static bool serveFile(const String& path, String& content);
  static bool writeWebFile(const String& path, const String& content);

private:
  static bool _atomicWrite(const String& path, const String& json);
  static String _readFile(const String& path);
};

// ── Updated UserProfile (admin_email added) ──────────────────
// Note: admin_email is now collected on the web registration page
// (index.html) and saved as part of both profile.json and network.json
// The field is added to the UserProfile struct:
//   String admin_email;   ← replaces what was in NetworkConfig::to_admin
