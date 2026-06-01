#pragma once
#include <Arduino.h>
enum class Lang : uint8_t { EN=0, AR, DE, FR, TR, ZH, _COUNT };
enum class Str  : uint8_t {
  OK,CANCEL,BACK,NEXT,YES,NO,SAVE,DELETE,
  LOCKED,ENTER_PIN,SCAN_FINGER,WRONG_PIN,WRONG_FP,
  AUTH_FAILED,PIN_MISMATCH,FP_SAVED,
  MENU_CREDS,MENU_PWGEN,MENU_SETTINGS,MENU_BACKUP,MENU_LOCK,
  SEND_BOTH,SEND_USER,SEND_PASS,TYPING,
  SET_PIN,CONFIRM_PIN,ENROLL_FP,ADD_FP,BACKUP_FREQ,
  SETUP_DONE,PLACE_FINGER,LIFT_REPLACE,
  STR_LANGUAGE,HID_MODE,AUTOLOCK,FP_MANAGE,BACKUP_NOW,ABOUT,
  USB_MODE,BLE_MODE,
  CONNECTING,CONNECTED,WIFI_FAIL,BACKUP_SENT,BACKUP_FAIL,
  BAT_LOW,BAT_CRITICAL,
  _COUNT
};
class Language {
public:
  static void        setLanguage(Lang l);
  static Lang        getLanguage();
  static bool        isRTL();
  static const char* get(Str key);
private:
  static Lang _lang;
};
