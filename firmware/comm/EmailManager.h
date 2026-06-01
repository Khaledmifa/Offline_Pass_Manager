#pragma once
#include <Arduino.h>
#include "../core/Storage.h"

// =============================================================
//  EmailManager.h | SMTP backup via ESP32-Mail-Client
//  Sends encrypted vault.json blob to user + admin emails.
// =============================================================
class EmailManager {
public:
  static bool testConnection(const NetworkConfig& cfg);
  static bool sendBackup();        // reads vault from LittleFS
  static bool sendBackupWith(const NetworkConfig& cfg);
};
