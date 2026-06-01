#pragma once
#include <Arduino.h>

// =============================================================
//  HID_Manager.h | Unified USB + BLE keyboard output
//  USB: USBHIDKeyboard (ESP32 Arduino Core 2.x, native USB S3)
//  BLE: NimBLE-Arduino HID profile
// =============================================================
enum class HIDMode : uint8_t { USB, BLE };

class HID_Manager {
public:
  static void    begin(HIDMode mode);
  static void    setMode(HIDMode mode);
  static HIDMode getMode();

  static void    typeString(const char* s);
  static void    typeUsername(const char* u);
  static void    typePassword(const char* p);
  static void    typeBoth(const char* u, const char* p);
  static void    pressTab();
  static void    pressEnter();
  static bool    isConnected();

private:
  static HIDMode _mode;
  static void    _typeChar(char c);
};
