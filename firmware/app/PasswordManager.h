#pragma once
#include <Arduino.h>
#include <vector>
#include "../core/Storage.h"
#include "../core/Security.h"

// =============================================================
//  PasswordManager.h | In-RAM credential list (post-unlock)
// =============================================================
struct CredPlain {
  String name;
  String username;
  String password;
};

class PasswordManager {
public:
  static bool   load();              // decrypt + load vault into RAM
  static void   unload();            // clear plaintext from RAM

  static uint8_t count();
  static const CredPlain& get(uint8_t idx);

  static bool   add(const String& name, const String& user, const String& pass);
  static bool   remove(uint8_t idx);
  static bool   update(uint8_t idx, const String& name,
                        const String& user, const String& pass);

  // HID dispatch
  static void   sendBoth(uint8_t idx);
  static void   sendUser(uint8_t idx);
  static void   sendPass(uint8_t idx);

private:
  static std::vector<CredPlain> _creds;
  static bool                   _loaded;
};
