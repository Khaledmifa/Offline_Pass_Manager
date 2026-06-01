#pragma once
#include <Arduino.h>
#include <mbedtls/aes.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/md.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

// =============================================================
//  Security.h | AES-256-CBC + PBKDF2-SHA256
//  Master key is derived from PIN at unlock time.
//  Key lives only in RAM and is zeroed on lock.
//  IV is random per operation, stored alongside ciphertext.
// =============================================================

class Security {
public:
  // Called once at first boot to generate a random salt
  static bool     generateSalt();
  static bool     loadSalt();       // load from config.json

  // Derive master key from PIN (call at unlock, zero on lock)
  static bool     deriveKey(const char* pin, uint8_t pinLen);
  static void     zeroKey();        // call on device lock

  static bool     isKeyReady();

  // AES-256-CBC encrypt / decrypt (output includes prepended IV)
  // Caller provides plaintext; function returns base64 blob = IV||ciphertext
  static String   encrypt(const uint8_t* plain, size_t plainLen);
  static bool     decrypt(const String& b64blob, uint8_t* out, size_t& outLen);

  // Convenience wrappers for strings
  static String   encryptStr(const String& s);
  static String   decryptStr(const String& b64blob);

  // PIN hash for verification (PBKDF2 of PIN with stored salt)
  static String   hashPIN(const char* pin, uint8_t len);
  static bool     verifyPIN(const char* pin, uint8_t len,
                             const String& storedHash);

  // Random helpers using hardware TRNG
  static void     randomBytes(uint8_t* buf, size_t n);

private:
  static uint8_t  _key[AES_KEY_BYTES];
  static uint8_t  _salt[PBKDF2_SALT_BYTES];
  static bool     _keyReady;
  static bool     _saltLoaded;

  static String   _b64encode(const uint8_t* data, size_t len);
  static bool     _b64decode(const String& s, uint8_t* out, size_t& outLen);
  static void     _pkcs7pad(uint8_t* buf, size_t dataLen,
                             size_t blockSize, size_t& paddedLen);
  static size_t   _pkcs7unpad(const uint8_t* buf, size_t len);
};

  // PIN & AP password strength validation
  static bool validatePINStrength(const char* pin, uint8_t len);
  static bool validateAPPassword();
