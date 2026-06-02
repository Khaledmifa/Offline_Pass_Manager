#pragma once
#include <Arduino.h>

// =============================================================
//  PasswordGenerator.h | Hardware TRNG password generation
// =============================================================
struct PwGenOptions {
  uint8_t length  = PWGEN_DEFAULT;
  bool    upper   = true;
  bool    lower   = true;
  bool    digits  = true;
  bool    symbols = true;
};

class PasswordGenerator {
public:
  static String   generate(const PwGenOptions& opts);
  static String   lastGenerated();
  static void     clear();
private:
  static String   _last;
};
