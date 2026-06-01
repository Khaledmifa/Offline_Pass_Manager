#include "PasswordManager.h"
#include "../comm/HID_Manager.h"
#include "../config.h"

std::vector<CredPlain> PasswordManager::_creds;
bool                   PasswordManager::_loaded = false;

bool PasswordManager::load() {
  std::vector<Credential> enc;
  if (!Storage::loadVault(enc)) return false;
  _creds.clear();
  for (auto& c : enc) {
    CredPlain p;
    p.name     = c.name;
    p.username = Security::decryptStr(c.username_enc);
    p.password = Security::decryptStr(c.password_enc);
    _creds.push_back(p);
  }
  _loaded = true;
  return true;
}

void PasswordManager::unload() {
  for (auto& c : _creds) {
    c.username = "";
    c.password = "";
  }
  _creds.clear();
  _loaded = false;
}

uint8_t PasswordManager::count() { return (uint8_t)_creds.size(); }

const CredPlain& PasswordManager::get(uint8_t idx) {
  static CredPlain empty;
  if (idx >= _creds.size()) return empty;
  return _creds[idx];
}

bool PasswordManager::add(const String& name,
                           const String& user, const String& pass) {
  Credential c;
  c.name         = name;
  c.username_enc = Security::encryptStr(user);
  c.password_enc = Security::encryptStr(pass);
  if (!Storage::appendCredential(c)) return false;
  CredPlain p { name, user, pass };
  _creds.push_back(p);
  return true;
}

bool PasswordManager::remove(uint8_t idx) {
  if (idx >= _creds.size()) return false;
  _creds.erase(_creds.begin() + idx);
  // Re-save entire vault
  std::vector<Credential> enc;
  for (auto& p : _creds) {
    Credential c;
    c.name         = p.name;
    c.username_enc = Security::encryptStr(p.username);
    c.password_enc = Security::encryptStr(p.password);
    enc.push_back(c);
  }
  return Storage::saveVault(enc);
}

bool PasswordManager::update(uint8_t idx, const String& name,
                              const String& user, const String& pass) {
  if (idx >= _creds.size()) return false;
  _creds[idx] = { name, user, pass };
  return remove(255); // trigger re-save without removing
}

void PasswordManager::sendBoth(uint8_t idx) {
  if (idx >= _creds.size()) return;
  HID_Manager::typeBoth(_creds[idx].username.c_str(),
                          _creds[idx].password.c_str());
}
void PasswordManager::sendUser(uint8_t idx) {
  if (idx >= _creds.size()) return;
  HID_Manager::typeUsername(_creds[idx].username.c_str());
}
void PasswordManager::sendPass(uint8_t idx) {
  if (idx >= _creds.size()) return;
  HID_Manager::typePassword(_creds[idx].password.c_str());
}
