#include "PasswordGenerator.h"
#include "../config.h"

String PasswordGenerator::_last = "";

String PasswordGenerator::generate(const PwGenOptions& opts) {
  String charset = "";
  if (opts.upper)   charset += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  if (opts.lower)   charset += "abcdefghijklmnopqrstuvwxyz";
  if (opts.digits)  charset += "0123456789";
  if (opts.symbols) charset += "!@#$%^&*()-_=+[]{}|;:,.<>?";
  if (charset.isEmpty()) charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  uint8_t len = constrain(opts.length, PWGEN_MIN, PWGEN_MAX);
  String result = "";
  result.reserve(len);
  uint8_t csLen = charset.length();
  for (uint8_t i = 0; i < len; i++) {
    // Hardware TRNG via esp_random()
    uint32_t r = esp_random();
    result += charset[r % csLen];
  }
  _last = result;
  return result;
}

String PasswordGenerator::lastGenerated() { return _last; }
void   PasswordGenerator::clear()         { _last = ""; }
