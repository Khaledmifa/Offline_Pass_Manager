#include "UI.h"
#include "../config.h"
#include "../hal/Display.h"
#include "../hal/Battery.h"
#include "../hal/Fingerprint.h"
#include "../core/Security.h"
#include "../core/Storage.h"
#include "../core/Language.h"
#include "../app/PasswordManager.h"
#include "../app/PasswordGenerator.h"
#include "../comm/HID_Manager.h"
#include "../comm/WiFiManager.h"
#include "../comm/EmailManager.h"

// ── Static member definitions ─────────────────────────────────
AppState UI::_state        = AppState::BOOT_SPLASH;
AppState UI::_prev         = AppState::BOOT_SPLASH;
uint32_t UI::_lastInput    = 0;
uint32_t UI::_autolock_ms  = AUTOLOCK_DEFAULT_MS;

uint8_t  UI::_menuSel      = 0;
uint8_t  UI::_menuScroll   = 0;
uint8_t  UI::_pinDigits[PIN_MAX_LEN] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t  UI::_pinActive    = 0;
uint8_t  UI::_pinLen       = PIN_MIN_LEN;
uint8_t  UI::_wrongPIN     = 0;
uint8_t  UI::_fpIndex      = 0;
uint8_t  UI::_fpScanStep   = 1;
uint8_t  UI::_credSel      = 0;
uint8_t  UI::_credScroll   = 0;
uint8_t  UI::_sendSel      = 0;
uint8_t  UI::_settingsSel  = 0;
uint8_t  UI::_langSel      = 0;
uint8_t  UI::_backupFreqSel= 2;   // default: weekly

// Persistent state loaded from Storage
static DeviceConfig gCfg;
static PwGenOptions gPwOpts;
static uint16_t     gPwScrollX = 0;
static bool         gFPFailed  = false;
static uint8_t      gBatPct    = 100;
static uint32_t     gBatCheck  = 0;

// ═══════════════════════════════════════════════════════════════
//  PUBLIC
// ═══════════════════════════════════════════════════════════════

void UI::begin() {
  _lastInput   = millis();
  Storage::loadConfig(gCfg);
  _autolock_ms = gCfg.autolock_ms;
  Language::setLanguage((Lang)gCfg.language);
  gBatPct = Battery::getPercent();
}

void UI::tick() {
  // ── Battery check every 10 s ──────────────────────────────
  if (millis() - gBatCheck > 10000) {
    gBatPct    = Battery::getPercent();
    gBatCheck  = millis();
    if (Battery::isCritical() && _state != AppState::BATTERY_CRITICAL &&
        _state != AppState::LOCKED_IDLE  && _state != AppState::BOOT_SPLASH) {
      setState(AppState::BATTERY_CRITICAL);
    }
  }

  // ── Auto-lock watchdog ────────────────────────────────────
  if (_state != AppState::LOCKED_IDLE   &&
      _state != AppState::BOOT_SPLASH   &&
      _state != AppState::FIRST_BOOT_AP &&
      _state != AppState::WIFI_EMAIL_AP &&
      _state != AppState::WIFI_CONNECTING &&
      _state != AppState::BATTERY_CRITICAL) {
    if (millis() - _lastInput > _autolock_ms) {
      setState(AppState::LOCKED_IDLE);
      return;
    }
  }

  // ── WiFi AP: process DNS ──────────────────────────────────
  if (_state == AppState::FIRST_BOOT_AP ||
      _state == AppState::WIFI_EMAIL_AP) {
    WiFiManager::processAP();
  }

  // ── Read joystick ─────────────────────────────────────────
  JoyEvent evt = Joystick::poll();
  if (evt != JoyEvent::NONE) resetWatchdog();

  // ── Dispatch to current state handler ─────────────────────
  switch (_state) {
    case AppState::BOOT_SPLASH:          _inputBootSplash(evt);       break;
    case AppState::FIRST_BOOT_AP:        _inputFirstBootAP(evt);      break;
    case AppState::WIZARD_LANGUAGE:      _inputWizardLanguage(evt);   break;
    case AppState::WIZARD_PIN_SET:       _inputWizardPinSet(evt);     break;
    case AppState::WIZARD_PIN_CONFIRM:   _inputWizardPinConfirm(evt); break;
    case AppState::WIZARD_FP_ENROLL:     _inputWizardFPEnroll(evt);   break;
    case AppState::WIZARD_FP_ANOTHER:    _inputWizardFPAnother(evt);  break;
    case AppState::WIZARD_BACKUP_FREQ:   _inputWizardBackupFreq(evt); break;
    case AppState::WIFI_EMAIL_AP:        _inputWifiEmailAP(evt);      break;
    case AppState::LOCKED_IDLE:          _inputLockedIdle(evt);       break;
    case AppState::UNLOCK_FINGERPRINT:   _inputUnlockFP(evt);         break;
    case AppState::UNLOCK_PIN:           _inputUnlockPIN(evt);        break;
    case AppState::MAIN_MENU:            _inputMainMenu(evt);         break;
    case AppState::CREDENTIALS_LIST:     _inputCredList(evt);         break;
    case AppState::CREDENTIAL_DETAIL:    _inputCredDetail(evt);       break;
    case AppState::CREDENTIAL_SEND:      _inputCredSend(evt);         break;
    case AppState::PW_GEN_OPTIONS:       _inputPWGenOptions(evt);     break;
    case AppState::PW_GEN_RESULT:        _inputPWGenResult(evt);      break;
    case AppState::SETTINGS_MENU:
    case AppState::SETTINGS_LANGUAGE:
    case AppState::SETTINGS_HID_MODE:
    case AppState::SETTINGS_AUTOLOCK:
    case AppState::SETTINGS_FP_MANAGE:
    case AppState::SETTINGS_ABOUT:       _inputSettings(evt);         break;
    default: break;
  }

  _render();
}

void UI::setState(AppState s) {
  _prev  = _state;
  _state = s;
  // onEnter actions
  switch (s) {
    case AppState::LOCKED_IDLE:
      PasswordManager::unload();
      Security::zeroKey();
      Display::sleep();
      break;
    case AppState::UNLOCK_FINGERPRINT:
      gFPFailed = false;
      Display::wake();
      Fingerprint::ledBlue();
      break;
    case AppState::UNLOCK_PIN:
      memset(_pinDigits, 0xFF, sizeof(_pinDigits));
      _pinActive = 0;
      _pinLen    = PIN_MIN_LEN;
      Display::wake();
      break;
    case AppState::MAIN_MENU:
      _menuSel = 0; _menuScroll = 0;
      Display::wake();
      break;
    case AppState::CREDENTIALS_LIST:
      _credSel = 0; _credScroll = 0;
      break;
    case AppState::WIZARD_FP_ENROLL:
      _fpScanStep = 1;
      break;
    case AppState::PW_GEN_OPTIONS:
      gPwOpts   = PwGenOptions{};
      gPwScrollX = 0;
      break;
    default: break;
  }
}

AppState UI::getState() { return _state; }

void UI::resetWatchdog() { _lastInput = millis(); }

// ═══════════════════════════════════════════════════════════════
//  INPUT HANDLERS
// ═══════════════════════════════════════════════════════════════

void UI::_inputBootSplash(JoyEvent e) {
  static uint32_t entered = 0;
  if (entered == 0) entered = millis();
  if (millis() - entered > 2500) {
    entered = 0;
    setState(Storage::isFirstBoot()
      ? AppState::FIRST_BOOT_AP
      : AppState::LOCKED_IDLE);
  }
}

void UI::_inputFirstBootAP(JoyEvent e) {
  // AP is running; web server handles registration.
  // UI waits until profile.json is written, then advances.
  static uint32_t check = 0;
  if (millis() - check > 2000) {
    check = millis();
    if (LittleFS.exists(PATH_PROFILE)) {
      WiFiManager::stopAP();
      setState(AppState::WIZARD_LANGUAGE);
    }
  }
}

void UI::_inputWizardLanguage(JoyEvent e) {
  const uint8_t N = (uint8_t)Lang::_COUNT;
  if (e == JoyEvent::DOWN)  { _langSel = (_langSel + 1) % N; }
  if (e == JoyEvent::UP)    { _langSel = (_langSel == 0) ? N-1 : _langSel-1; }
  if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
    Language::setLanguage((Lang)_langSel);
    gCfg.language = _langSel;
    setState(AppState::WIZARD_PIN_SET);
  }
  // Scroll adjustment
  if (_langSel >= _menuScroll + 4) _menuScroll = _langSel - 3;
  if (_langSel < _menuScroll)      _menuScroll = _langSel;
}

void UI::_inputWizardPinSet(JoyEvent e) {
  if (e == JoyEvent::UP)   { _pinDigits[_pinActive] = (_pinDigits[_pinActive] + 1) % 10; }
  if (e == JoyEvent::DOWN) {
    _pinDigits[_pinActive] = (_pinDigits[_pinActive] == 0) ? 9
                             : _pinDigits[_pinActive] - 1;
  }
  if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
    if (_pinDigits[_pinActive] == 0xFF) _pinDigits[_pinActive] = 0;
    if (_pinActive < _pinLen - 1) { _pinActive++; }
    else {
      // All digits entered → move to confirm
      _pinActive = 0;
      setState(AppState::WIZARD_PIN_CONFIRM);
    }
  }
  if (e == JoyEvent::LEFT && _pinActive > 0) {
    _pinActive--;
    _pinDigits[_pinActive] = 0xFF;
  }
}

void UI::_inputWizardPinConfirm(JoyEvent e) {
  static uint8_t confirm[PIN_MAX_LEN];
  if (_pinActive == 0 && e == JoyEvent::NONE) {
    memset(confirm, 0xFF, sizeof(confirm));
  }
  if (e == JoyEvent::UP)   { confirm[_pinActive] = (confirm[_pinActive] + 1) % 10; }
  if (e == JoyEvent::DOWN) {
    confirm[_pinActive] = (confirm[_pinActive] == 0) ? 9 : confirm[_pinActive] - 1;
  }
  if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
    if (confirm[_pinActive] == 0xFF) confirm[_pinActive] = 0;
    if (_pinActive < _pinLen - 1) { _pinActive++; }
    else {
      // Verify match
      bool match = (memcmp(_pinDigits, confirm, _pinLen) == 0);
      if (!match) {
        Display::drawMessage("PIN Mismatch", "Please try again", true);
        delay(1500);
        _pinActive = 0;
        memset(confirm, 0xFF, sizeof(confirm));
        memset(_pinDigits, 0xFF, sizeof(_pinDigits));
        setState(AppState::WIZARD_PIN_SET);
        return;
      }
      // Hash and save
      char pinStr[PIN_MAX_LEN + 1];
      for (uint8_t i = 0; i < _pinLen; i++) pinStr[i] = '0' + _pinDigits[i];
      pinStr[_pinLen] = 0;
      Security::generateSalt();
      gCfg.pinHash = Security::hashPIN(pinStr, _pinLen);
      Storage::saveConfig(gCfg);
      _pinActive = 0;
      _fpIndex   = 0;
      setState(AppState::WIZARD_FP_ENROLL);
    }
  }
  if (e == JoyEvent::LEFT) {
    if (_pinActive > 0) { _pinActive--; confirm[_pinActive] = 0xFF; }
    else { setState(AppState::WIZARD_PIN_SET); }
  }
}

void UI::_inputWizardFPEnroll(JoyEvent e) {
  static uint32_t scanStart = 0;
  if (scanStart == 0) scanStart = millis();

  if (_fpScanStep == 1) {
    FPResult r = Fingerprint::enrollStep1(_fpIndex);
    if (r == FPResult::ENROLL_STEP1_DONE) { _fpScanStep = 2; scanStart = millis(); }
    else if (r == FPResult::TIMEOUT)       {
      Display::drawMessage("Timeout", "Place finger faster"); delay(1200);
    }
  } else {
    FPResult r = Fingerprint::enrollStep2(_fpIndex);
    if (r == FPResult::OK) {
      gCfg.fpSlots[_fpIndex] = true;
      gCfg.fpSlotCount = _fpIndex + 1;
      scanStart = 0;
      setState(AppState::WIZARD_FP_ANOTHER);
    } else if (r == FPResult::FAIL) {
      Display::drawMessage("Scan failed", "Try again"); delay(1200);
      _fpScanStep = 1; scanStart = 0;
    }
  }
}

void UI::_inputWizardFPAnother(JoyEvent e) {
  if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {  // YES
    if (_fpIndex + 1 < FP_MAX_SLOTS) {
      _fpIndex++;
      _fpScanStep = 1;
      setState(AppState::WIZARD_FP_ENROLL);
    } else {
      setState(AppState::WIZARD_BACKUP_FREQ);
    }
  }
  if (e == JoyEvent::LEFT) {   // NO
    setState(AppState::WIZARD_BACKUP_FREQ);
  }
}

void UI::_inputWizardBackupFreq(JoyEvent e) {
  const uint8_t N = 5;
  if (e == JoyEvent::UP)   { _backupFreqSel = (_backupFreqSel == 0) ? N-1 : _backupFreqSel-1; }
  if (e == JoyEvent::DOWN) { _backupFreqSel = (_backupFreqSel + 1) % N; }
  if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
    const uint8_t freqs[] = {1, 3, 7, 14, 30};
    gCfg.backup_freq_days = freqs[_backupFreqSel];
    gCfg.setupDone = true;
    Storage::saveConfig(gCfg);
    // Move to WiFi+Email setup AP
    Display::drawSetupDone(); delay(2000);
    WiFiManager::startAP(APPhase::NETWORK_SETUP);
    setState(AppState::WIFI_EMAIL_AP);
  }
}

void UI::_inputWifiEmailAP(JoyEvent e) {
  // Wait for network.json to be written by web handler
  static uint32_t check = 0;
  if (millis() - check > 2000) {
    check = millis();
    if (LittleFS.exists(PATH_NETWORK)) {
      WiFiManager::stopAP();
      NetworkConfig cfg;
      Storage::loadNetwork(cfg);
      Display::drawWiFiConnecting(cfg.wifi_ssid.c_str());
      bool ok = WiFiManager::connectSTA(cfg.wifi_ssid, cfg.wifi_pass, 15000);
      gCfg.wifiSetupDone = ok;
      Storage::saveConfig(gCfg);
      Display::drawSetupDone(); delay(2000);
      setState(AppState::LOCKED_IDLE);
    }
  }
}

void UI::_inputLockedIdle(JoyEvent e) {
  if (e == JoyEvent::NONE) return;
  Display::wake();
  if (e == JoyEvent::PRESS || e == JoyEvent::LEFT) {
    setState(AppState::UNLOCK_PIN);
  } else {
    setState(AppState::UNLOCK_FINGERPRINT);
  }
}

void UI::_inputUnlockFP(JoyEvent e) {
  if (e == JoyEvent::LEFT) { setState(AppState::UNLOCK_PIN); return; }

  static uint32_t scanStart = 0;
  if (scanStart == 0) scanStart = millis();

  uint8_t slot = 0;
  FPResult r   = Fingerprint::verify(slot);

  if (r == FPResult::OK) {
    // Derive key — prompt for PIN if only one attempt
    // In fingerprint mode, we still need the PIN to derive the AES key.
    // Solution: store a "device key" encrypted with the fingerprint's slot hash.
    // For v1.0: fingerprint unlock requires PIN once per session for key derivation.
    scanStart = 0;
    setState(AppState::UNLOCK_PIN);   // derive key from PIN
    gFPFailed = false;
  } else if (r == FPResult::FAIL) {
    gFPFailed = true;
    scanStart = 0;
    // Allow 3 FP failures then require PIN
    static uint8_t fpFails = 0;
    fpFails++;
    if (fpFails >= 3) { fpFails = 0; setState(AppState::UNLOCK_PIN); }
  } else if (r == FPResult::TIMEOUT) {
    scanStart = 0;
    setState(AppState::LOCKED_IDLE);
  }
}

void UI::_inputUnlockPIN(JoyEvent e) {
  if (e == JoyEvent::LEFT && _pinActive == 0) {
    setState(AppState::LOCKED_IDLE); return;
  }
  if (e == JoyEvent::UP)   {
    if (_pinDigits[_pinActive] == 0xFF) _pinDigits[_pinActive] = 0;
    else _pinDigits[_pinActive] = (_pinDigits[_pinActive] + 1) % 10;
  }
  if (e == JoyEvent::DOWN) {
    if (_pinDigits[_pinActive] == 0xFF) _pinDigits[_pinActive] = 9;
    else _pinDigits[_pinActive] = (_pinDigits[_pinActive] == 0) ? 9 : _pinDigits[_pinActive]-1;
  }
  if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
    if (_pinDigits[_pinActive] == 0xFF) _pinDigits[_pinActive] = 0;
    if (_pinActive < _pinLen - 1) { _pinActive++; return; }
    // All digits entered — verify
    char pinStr[PIN_MAX_LEN + 1];
    for (uint8_t i = 0; i < _pinLen; i++) pinStr[i] = '0' + _pinDigits[i];
    pinStr[_pinLen] = 0;
    if (!Security::verifyPIN(pinStr, _pinLen, gCfg.pinHash)) {
      _wrongPIN++;
      memset(_pinDigits, 0xFF, sizeof(_pinDigits));
      _pinActive = 0;
      if (_wrongPIN >= PIN_MAX_ATTEMPTS) {
        _wrongPIN = 0;
        Display::drawMessage("Too many attempts", "Locked 30s", true);
        delay(30000);
        setState(AppState::LOCKED_IDLE);
      }
      return;
    }
    // PIN correct
    _wrongPIN = 0;
    Security::deriveKey(pinStr, _pinLen);
    PasswordManager::load();
    setState(AppState::MAIN_MENU);
  }
  if (e == JoyEvent::LEFT && _pinActive > 0) {
    _pinDigits[_pinActive] = 0xFF;
    _pinActive--;
  }
}

void UI::_inputMainMenu(JoyEvent e) {
  const uint8_t N = 5;
  if (e == JoyEvent::UP)   { if (_menuSel > 0) _menuSel--; }
  if (e == JoyEvent::DOWN) { if (_menuSel < N-1) _menuSel++; }
  // Keep scroll window around selection
  if (_menuSel >= _menuScroll + 4) _menuScroll++;
  if (_menuSel < _menuScroll)      _menuScroll--;

  if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
    switch (_menuSel) {
      case 0: setState(AppState::CREDENTIALS_LIST); break;
      case 1: setState(AppState::PW_GEN_OPTIONS);   break;
      case 2: setState(AppState::SETTINGS_MENU);    break;
      case 3:   // Backup now
        Display::drawProgress("Sending backup...", 50);
        EmailManager::sendBackup();
        Display::drawMessage("Backup", "Email sent!"); delay(2000);
        break;
      case 4: setState(AppState::LOCKED_IDLE); break;
    }
  }
}

void UI::_inputCredList(JoyEvent e) {
  uint8_t N = PasswordManager::count();
  if (e == JoyEvent::LEFT)  { setState(AppState::MAIN_MENU); return; }
  if (e == JoyEvent::UP)    { if (_credSel > 0) _credSel--; }
  if (e == JoyEvent::DOWN)  { if (_credSel < N-1) _credSel++; }
  if (_credSel >= _credScroll + 4) _credScroll++;
  if (_credSel < _credScroll)      _credScroll--;
  if ((e == JoyEvent::RIGHT || e == JoyEvent::PRESS) && N > 0) {
    setState(AppState::CREDENTIAL_DETAIL);
  }
}

void UI::_inputCredDetail(JoyEvent e) {
  if (e == JoyEvent::LEFT)                           { setState(AppState::CREDENTIALS_LIST); }
  if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS)  { _sendSel = 0; setState(AppState::CREDENTIAL_SEND); }
}

void UI::_inputCredSend(JoyEvent e) {
  if (e == JoyEvent::LEFT)  { setState(AppState::CREDENTIAL_DETAIL); return; }
  if (e == JoyEvent::UP)    { if (_sendSel > 0) _sendSel--; }
  if (e == JoyEvent::DOWN)  { if (_sendSel < 2) _sendSel++; }
  if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
    setState(AppState::CREDENTIAL_SENDING);
    Display::drawSending("Typing...");
    switch (_sendSel) {
      case 0: PasswordManager::sendBoth(_credSel); break;
      case 1: PasswordManager::sendUser(_credSel); break;
      case 2: PasswordManager::sendPass(_credSel); break;
    }
    delay(300);
    setState(AppState::CREDENTIAL_DETAIL);
  }
}

void UI::_inputPWGenOptions(JoyEvent e) {
  static uint8_t focus = 0;  // 0=length, 1=upper, 2=lower, 3=digits, 4=symbols
  const uint8_t FIELDS = 5;
  if (e == JoyEvent::LEFT)  { setState(AppState::MAIN_MENU); return; }
  if (e == JoyEvent::UP)    { focus = (focus == 0) ? FIELDS-1 : focus-1; }
  if (e == JoyEvent::DOWN)  { focus = (focus + 1) % FIELDS; }
  if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
    switch (focus) {
      case 0: gPwOpts.length = constrain(gPwOpts.length + 4, PWGEN_MIN, PWGEN_MAX); break;
      case 1: gPwOpts.upper   = !gPwOpts.upper;   break;
      case 2: gPwOpts.lower   = !gPwOpts.lower;   break;
      case 3: gPwOpts.digits  = !gPwOpts.digits;  break;
      case 4: gPwOpts.symbols = !gPwOpts.symbols; break;
    }
  }
  // Long press (held ►) → generate
  // For simplicity: pressing on length field generates
  if (focus == 0 && (e == JoyEvent::PRESS)) {
    PasswordGenerator::generate(gPwOpts);
    gPwScrollX = 0;
    setState(AppState::PW_GEN_RESULT);
  }
}

void UI::_inputPWGenResult(JoyEvent e) {
  if (e == JoyEvent::LEFT)  { setState(AppState::PW_GEN_OPTIONS); return; }
  if (e == JoyEvent::DOWN)  { // scroll password right
    gPwScrollX = min((uint16_t)(gPwScrollX + 6), (uint16_t)200);
  }
  if (e == JoyEvent::UP)    { // scroll left
    gPwScrollX = (gPwScrollX > 6) ? gPwScrollX - 6 : 0;
  }
  if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
    // Save to new credential (name = "Generated")
    String pw = PasswordGenerator::lastGenerated();
    PasswordManager::add("Generated", "", pw);
    Display::drawMessage("Saved!", "New entry created"); delay(1500);
    setState(AppState::CREDENTIALS_LIST);
  }
}

void UI::_inputSettings(JoyEvent e) {
  const uint8_t N = 7;
  static const char* items[] = {
    "Language", "HID Mode", "Auto-Lock", "Fingerprints",
    "Backup Now", "About", "< Back"
  };

  if (_state == AppState::SETTINGS_MENU) {
    if (e == JoyEvent::LEFT) { setState(AppState::MAIN_MENU); return; }
    if (e == JoyEvent::UP)   { if (_settingsSel > 0) _settingsSel--; }
    if (e == JoyEvent::DOWN) { if (_settingsSel < N-1) _settingsSel++; }
    if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
      switch (_settingsSel) {
        case 0: _langSel = gCfg.language; setState(AppState::SETTINGS_LANGUAGE); break;
        case 1: setState(AppState::SETTINGS_HID_MODE);  break;
        case 2: setState(AppState::SETTINGS_AUTOLOCK);  break;
        case 3: setState(AppState::SETTINGS_FP_MANAGE); break;
        case 4:
          Display::drawProgress("Sending backup...", 50);
          EmailManager::sendBackup();
          Display::drawMessage("Backup", "Email sent!"); delay(2000);
          break;
        case 5: setState(AppState::SETTINGS_ABOUT);     break;
        case 6: setState(AppState::MAIN_MENU);           break;
      }
    }
    return;
  }

  if (_state == AppState::SETTINGS_LANGUAGE) {
    const uint8_t LN = (uint8_t)Lang::_COUNT;
    if (e == JoyEvent::LEFT)  { setState(AppState::SETTINGS_MENU); return; }
    if (e == JoyEvent::UP)    { _langSel = (_langSel == 0) ? LN-1 : _langSel-1; }
    if (e == JoyEvent::DOWN)  { _langSel = (_langSel + 1) % LN; }
    if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
      gCfg.language = _langSel;
      Language::setLanguage((Lang)_langSel);
      Storage::saveConfig(gCfg);
      setState(AppState::SETTINGS_MENU);
    }
    return;
  }

  if (_state == AppState::SETTINGS_HID_MODE) {
    if (e == JoyEvent::LEFT)  { setState(AppState::SETTINGS_MENU); return; }
    if (e == JoyEvent::UP || e == JoyEvent::DOWN) { gCfg.hid_ble = !gCfg.hid_ble; }
    if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
      Storage::saveConfig(gCfg);
      HID_Manager::setMode(gCfg.hid_ble ? HIDMode::BLE : HIDMode::USB);
      setState(AppState::SETTINGS_MENU);
    }
    return;
  }

  if (_state == AppState::SETTINGS_AUTOLOCK) {
    static const uint32_t times[] = {15000,30000,60000,120000,300000};
    static uint8_t sel = 1;
    if (e == JoyEvent::LEFT)  { setState(AppState::SETTINGS_MENU); return; }
    if (e == JoyEvent::UP)    { if (sel > 0) sel--; }
    if (e == JoyEvent::DOWN)  { if (sel < 4) sel++; }
    if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
      gCfg.autolock_ms = times[sel];
      _autolock_ms     = times[sel];
      Storage::saveConfig(gCfg);
      setState(AppState::SETTINGS_MENU);
    }
    return;
  }

  if (_state == AppState::SETTINGS_ABOUT) {
    if (e != JoyEvent::NONE) setState(AppState::SETTINGS_MENU);
    return;
  }

  // SETTINGS_FP_MANAGE — basic: show count, option to delete all
  if (_state == AppState::SETTINGS_FP_MANAGE) {
    if (e == JoyEvent::LEFT) { setState(AppState::SETTINGS_MENU); return; }
    if (e == JoyEvent::PRESS) {
      Fingerprint::deleteAll();
      memset(gCfg.fpSlots, 0, sizeof(gCfg.fpSlots));
      gCfg.fpSlotCount = 0;
      Storage::saveConfig(gCfg);
      Display::drawMessage("Done", "All FP deleted"); delay(1500);
      setState(AppState::SETTINGS_MENU);
    }
    return;
  }

  // BATTERY_CRITICAL overlay
  if (_state == AppState::BATTERY_CRITICAL) {
    if (e != JoyEvent::NONE) setState(_prev);
    return;
  }
}

// ═══════════════════════════════════════════════════════════════
//  RENDER
// ═══════════════════════════════════════════════════════════════

void UI::_render() {
  static const char* mainItems[]     = {"Credentials","PW Generator","Settings","Backup Now","Lock"};
  static const char* langNames[]     = {"English","العربية","Deutsch","Français","Türkçe","中文"};
  static const char* settingsItems[] = {"Language","HID Mode","Auto-Lock","Fingerprints","Backup Now","About","< Back"};

  switch (_state) {
    case AppState::BOOT_SPLASH:
      Display::drawBootSplash(FW_VERSION); break;

    case AppState::FIRST_BOOT_AP:
    case AppState::WIFI_EMAIL_AP:
      Display::drawSetupAP(SETUP_AP_SSID, SETUP_AP_IP); break;

    case AppState::WIZARD_LANGUAGE:
      Display::drawLanguageSelect(langNames, 6, _langSel, _menuScroll); break;

    case AppState::WIZARD_PIN_SET:
      Display::drawPINEntry(_pinDigits, _pinLen, _pinActive, false, false); break;

    case AppState::WIZARD_PIN_CONFIRM:
      Display::drawPINEntry(_pinDigits, _pinLen, _pinActive, true, false); break;

    case AppState::WIZARD_FP_ENROLL:
      Display::drawFPEnroll(_fpScanStep, _fpIndex); break;

    case AppState::WIZARD_FP_ANOTHER:
      Display::drawFPAnother(gCfg.fpSlotCount); break;

    case AppState::WIZARD_BACKUP_FREQ:
      Display::drawBackupFreq(_backupFreqSel); break;

    case AppState::WIFI_CONNECTING:
      Display::drawWiFiConnecting("..."); break;

    case AppState::LOCKED_IDLE:
      // OLED is sleeping; render only wakes it
      break;

    case AppState::UNLOCK_FINGERPRINT:
      Display::drawUnlockFP(true, gFPFailed); break;

    case AppState::UNLOCK_PIN:
      Display::drawUnlockPIN(_pinDigits, _pinLen, _pinActive, _wrongPIN); break;

    case AppState::MAIN_MENU:
      Display::drawMainMenu(mainItems, 5, _menuSel, _menuScroll, gBatPct, gCfg.hid_ble); break;

    case AppState::CREDENTIALS_LIST: {
      uint8_t n = PasswordManager::count();
      static const char* names[200];
      for (uint8_t i = 0; i < n; i++) names[i] = PasswordManager::get(i).name.c_str();
      Display::drawCredList(names, n, _credSel, _credScroll);
      break;
    }
    case AppState::CREDENTIAL_DETAIL: {
      const CredPlain& c = PasswordManager::get(_credSel);
      Display::drawCredDetail(c.name.c_str(), c.username.c_str()); break;
    }
    case AppState::CREDENTIAL_SEND:
      Display::drawSendMenu(_sendSel); break;

    case AppState::CREDENTIAL_SENDING:
      Display::drawSending("Typing..."); break;

    case AppState::PW_GEN_OPTIONS:
      Display::drawPWGenOptions(gPwOpts.length, gPwOpts.upper, gPwOpts.lower,
                                 gPwOpts.digits, gPwOpts.symbols, 0); break;

    case AppState::PW_GEN_RESULT:
      Display::drawPWGenResult(PasswordGenerator::lastGenerated().c_str(), gPwScrollX); break;

    case AppState::SETTINGS_MENU:
      Display::drawSettings(settingsItems, 7, _settingsSel, 0); break;

    case AppState::SETTINGS_LANGUAGE:
      Display::drawLanguageSelect(langNames, 6, _langSel, 0); break;

    case AppState::SETTINGS_ABOUT:
      Display::drawAbout(FW_VERSION, gBatPct); break;

    case AppState::BATTERY_CRITICAL:
      Display::drawBatteryLow(gBatPct); break;

    default: break;
  }
}
