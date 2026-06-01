#pragma once
#include <Arduino.h>
#include "../hal/Joystick.h"

// =============================================================
//  UI.h | Central Finite State Machine
//  Dispatches joystick events and renders to OLED each tick.
// =============================================================

enum class AppState : uint8_t {
  BOOT_SPLASH,
  FIRST_BOOT_AP, FIRST_BOOT_WEB_DONE,
  WIZARD_LANGUAGE, WIZARD_PIN_SET, WIZARD_PIN_CONFIRM,
  WIZARD_FP_ENROLL, WIZARD_FP_ANOTHER, WIZARD_BACKUP_FREQ,
  WIFI_EMAIL_AP, WIFI_CONNECTING,
  LOCKED_IDLE,
  UNLOCK_FINGERPRINT, UNLOCK_PIN,
  MAIN_MENU,
  CREDENTIALS_LIST, CREDENTIAL_DETAIL, CREDENTIAL_SEND, CREDENTIAL_SENDING,
  PW_GEN_OPTIONS, PW_GEN_RESULT,
  SETTINGS_MENU, SETTINGS_LANGUAGE, SETTINGS_HID_MODE,
  SETTINGS_AUTOLOCK, SETTINGS_FP_MANAGE, SETTINGS_ABOUT,
  BATTERY_CRITICAL
};

class UI {
public:
  static void       begin();
  static void       tick();           // call every loop() iteration
  static void       setState(AppState s);
  static AppState   getState();
  static void       resetWatchdog();  // call on any user input

private:
  static AppState   _state;
  static AppState   _prev;
  static uint32_t   _lastInput;
  static uint32_t   _autolock_ms;

  // Per-state input handlers
  static void _inputBootSplash(JoyEvent e);
  static void _inputFirstBootAP(JoyEvent e);
  static void _inputWizardLanguage(JoyEvent e);
  static void _inputWizardPinSet(JoyEvent e);
  static void _inputWizardPinConfirm(JoyEvent e);
  static void _inputWizardFPEnroll(JoyEvent e);
  static void _inputWizardFPAnother(JoyEvent e);
  static void _inputWizardBackupFreq(JoyEvent e);
  static void _inputWifiEmailAP(JoyEvent e);
  static void _inputLockedIdle(JoyEvent e);
  static void _inputUnlockFP(JoyEvent e);
  static void _inputUnlockPIN(JoyEvent e);
  static void _inputMainMenu(JoyEvent e);
  static void _inputCredList(JoyEvent e);
  static void _inputCredDetail(JoyEvent e);
  static void _inputCredSend(JoyEvent e);
  static void _inputPWGenOptions(JoyEvent e);
  static void _inputPWGenResult(JoyEvent e);
  static void _inputSettings(JoyEvent e);

  // Per-state renderers
  static void _render();

  // State variables
  static uint8_t _menuSel;
  static uint8_t _menuScroll;
  static uint8_t _pinDigits[PIN_MAX_LEN];
  static uint8_t _pinActive;
  static uint8_t _pinLen;
  static uint8_t _wrongPIN;
  static uint8_t _fpIndex;
  static uint8_t _fpScanStep;
  static uint8_t _credSel;
  static uint8_t _credScroll;
  static uint8_t _sendSel;
  static uint8_t _settingsSel;
  static uint8_t _langSel;
  static uint8_t _backupFreqSel;
};
