#include "Security.h"
#include "Storage.h"
#include "../config.h"
#include <base64.h>      // ESP32 built-in base64 (mbedtls)
#include <string.h>

// Static member definitions
uint8_t Security::_key[AES_KEY_BYTES]         = {};
uint8_t Security::_salt[PBKDF2_SALT_BYTES]    = {};
bool    Security::_keyReady   = false;
bool    Security::_saltLoaded = false;

// ── Random (hardware TRNG on ESP32-S3) ────────────────────────
void Security::randomBytes(uint8_t* buf, size_t n) {
  for (size_t i = 0; i < n; i += 4) {
    uint32_t r = esp_random();
    size_t copy = min((size_t)4, n - i);
    memcpy(buf + i, &r, copy);
  }
}

// ── Salt management ───────────────────────────────────────────
bool Security::generateSalt() {
  randomBytes(_salt, PBKDF2_SALT_BYTES);
  _saltLoaded = true;
  return Storage::saveSalt(_salt, PBKDF2_SALT_BYTES);
}

bool Security::loadSalt() {
  bool ok = Storage::loadSalt(_salt, PBKDF2_SALT_BYTES);
  if (ok) _saltLoaded = true;
  return ok;
}

// ── Key derivation ────────────────────────────────────────────
bool Security::deriveKey(const char* pin, uint8_t pinLen) {
  if (!_saltLoaded) return false;
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_setup(&ctx, info, 1);
  int ret = mbedtls_pkcs5_pbkdf2_hmac(&ctx,
              (const uint8_t*)pin, pinLen,
              _salt, PBKDF2_SALT_BYTES,
              PBKDF2_ROUNDS,
              AES_KEY_BYTES, _key);
  mbedtls_md_free(&ctx);
  _keyReady = (ret == 0);
  return _keyReady;
}

void Security::zeroKey() {
  memset(_key, 0, AES_KEY_BYTES);
  _keyReady = false;
}

bool Security::isKeyReady() { return _keyReady; }

// ── PKCS#7 padding ────────────────────────────────────────────
void Security::_pkcs7pad(uint8_t* buf, size_t dataLen,
                          size_t blockSize, size_t& paddedLen) {
  uint8_t pad = blockSize - (dataLen % blockSize);
  for (size_t i = dataLen; i < dataLen + pad; i++) buf[i] = pad;
  paddedLen = dataLen + pad;
}

size_t Security::_pkcs7unpad(const uint8_t* buf, size_t len) {
  if (len == 0) return 0;
  uint8_t pad = buf[len - 1];
  if (pad == 0 || pad > 16) return len;
  return len - pad;
}

// ── Base64 helpers ────────────────────────────────────────────
String Security::_b64encode(const uint8_t* data, size_t len) {
  return base64::encode(data, len);
}

bool Security::_b64decode(const String& s, uint8_t* out, size_t& outLen) {
  String dec = base64::decode(s);
  outLen = dec.length();
  memcpy(out, dec.c_str(), outLen);
  return true;
}

// ── Encrypt: returns base64(IV || ciphertext) ─────────────────
String Security::encrypt(const uint8_t* plain, size_t plainLen) {
  if (!_keyReady) return "";
  const size_t BLOCK = 16;
  size_t paddedLen = 0;
  uint8_t* padded = new uint8_t[plainLen + BLOCK + 1];
  memcpy(padded, plain, plainLen);
  _pkcs7pad(padded, plainLen, BLOCK, paddedLen);

  uint8_t iv[AES_IV_BYTES];
  randomBytes(iv, AES_IV_BYTES);

  uint8_t* cipher = new uint8_t[paddedLen];
  uint8_t iv_cpy[AES_IV_BYTES];
  memcpy(iv_cpy, iv, AES_IV_BYTES);

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, _key, AES_KEY_BYTES * 8);
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT,
                         paddedLen, iv_cpy, padded, cipher);
  mbedtls_aes_free(&aes);

  // Prepend IV to ciphertext
  uint8_t* combined = new uint8_t[AES_IV_BYTES + paddedLen];
  memcpy(combined,               iv,     AES_IV_BYTES);
  memcpy(combined + AES_IV_BYTES, cipher, paddedLen);

  String result = _b64encode(combined, AES_IV_BYTES + paddedLen);

  delete[] padded; delete[] cipher; delete[] combined;
  return result;
}

// ── Decrypt: input is base64(IV || ciphertext) ────────────────
bool Security::decrypt(const String& b64blob, uint8_t* out, size_t& outLen) {
  if (!_keyReady) return false;
  const size_t BLOCK = 16;
  uint8_t buf[2048]; size_t bufLen = 0;
  if (!_b64decode(b64blob, buf, bufLen)) return false;
  if (bufLen <= AES_IV_BYTES) return false;

  uint8_t iv[AES_IV_BYTES];
  memcpy(iv, buf, AES_IV_BYTES);
  size_t cipherLen = bufLen - AES_IV_BYTES;

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_dec(&aes, _key, AES_KEY_BYTES * 8);
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT,
                         cipherLen, iv, buf + AES_IV_BYTES, out);
  mbedtls_aes_free(&aes);

  outLen = _pkcs7unpad(out, cipherLen);
  out[outLen] = 0;
  return true;
}

String Security::encryptStr(const String& s) {
  return encrypt((const uint8_t*)s.c_str(), s.length());
}

String Security::decryptStr(const String& b64blob) {
  uint8_t out[2048]; size_t outLen = 0;
  if (!decrypt(b64blob, out, outLen)) return "";
  return String((char*)out);
}

// ── PIN hash (PBKDF2 with same salt, fewer rounds for speed) ──
String Security::hashPIN(const char* pin, uint8_t len) {
  if (!_saltLoaded) return "";
  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_setup(&ctx, info, 1);
  mbedtls_pkcs5_pbkdf2_hmac(&ctx,
    (const uint8_t*)pin, len, _salt, PBKDF2_SALT_BYTES,
    10000, 32, hash);
  mbedtls_md_free(&ctx);
  return _b64encode(hash, 32);
}

bool Security::verifyPIN(const char* pin, uint8_t len, const String& stored) {
  return hashPIN(pin, len) == stored;
}

// ── PIN strength validation ───────────────────────────────────
// Called before accepting a new PIN during wizard or PIN change.
// Rules: min 8 chars, ≥1 uppercase, ≥1 lowercase, ≥1 digit, ≥1 symbol, ASCII only.
bool Security::validatePINStrength(const char* pin, uint8_t len) {
  if (len < PIN_MIN_LEN) return false;
  bool hasUpper = false, hasLower = false, hasDigit = false, hasSymbol = false;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t c = (uint8_t)pin[i];
    if (c > 127) return false;            // ASCII only
    if (c >= 'A' && c <= 'Z') hasUpper  = true;
    else if (c >= 'a' && c <= 'z') hasLower  = true;
    else if (c >= '0' && c <= '9') hasDigit  = true;
    else                            hasSymbol = true;
  }
  return hasUpper && hasLower && hasDigit && hasSymbol;
}

// ── AP password strength check (compile-time validated via runtime check) ──
bool Security::validateAPPassword() {
  const char* p = SETUP_AP_PASS;
  uint8_t len = strlen(p);
  if (len < 10) return false;
  bool u=false,l=false,d=false,s=false;
  for (uint8_t i=0;i<len;i++){
    char c=p[i];
    if(c>='A'&&c<='Z')u=true;
    else if(c>='a'&&c<='z')l=true;
    else if(c>='0'&&c<='9')d=true;
    else s=true;
  }
  return u&&l&&d&&s;
}
