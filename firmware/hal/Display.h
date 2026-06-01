#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
// =============================================================
//  Display.h | SH1107 64x128 OLED — landscape via U8G2_R1
//  After rotation: 128px wide x 64px tall
//  Layout: [0..10] status bar | [12..63] content
// =============================================================

// Menu item descriptor
struct MenuItem {
  const char* label;
  bool        selected;
};

class Display {
public:
  static void begin();
  static void sleep();
  static void wake();

  // ── Status bar (always drawn) ──────────────────────────────
  static void drawStatusBar(uint8_t batPct, bool charging,
                             bool bleMode, bool locked);

  // ── Setup / wizard screens ────────────────────────────────
  static void drawBootSplash(const char* version);
  static void drawSetupAP(const char* ssid, const char* ip);
  static void drawWizardProgress(uint8_t step, uint8_t total);
  static void drawLanguageSelect(const char* langs[], uint8_t count,
                                 uint8_t sel, uint8_t scroll);
  static void drawPINEntry(const uint8_t* digits, uint8_t len,
                           uint8_t active, bool isConfirm, bool isWrong);
  static void drawFPEnroll(uint8_t scanStep, uint8_t fpIndex);
  static void drawFPAnother(uint8_t enrolled);
  static void drawBackupFreq(uint8_t sel);
  static void drawWiFiConnecting(const char* ssid);
  static void drawSetupDone();

  // ── Lock screen ───────────────────────────────────────────
  static void drawLocked(uint8_t batPct, bool charging);
  static void drawUnlockFP(bool scanning, bool failed);
  static void drawUnlockPIN(const uint8_t* digits, uint8_t len,
                             uint8_t active, uint8_t wrongCount);

  // ── Main menu ─────────────────────────────────────────────
  static void drawMainMenu(const char* items[], uint8_t count,
                            uint8_t sel, uint8_t scroll,
                            uint8_t batPct, bool ble);

  // ── Credentials ───────────────────────────────────────────
  static void drawCredList(const char* names[], uint8_t count,
                            uint8_t sel, uint8_t scroll);
  static void drawCredDetail(const char* name, const char* user);
  static void drawSendMenu(uint8_t sel);
  static void drawSending(const char* label);

  // ── Password generator ────────────────────────────────────
  static void drawPWGenOptions(uint8_t len, bool upper, bool lower,
                                bool digits, bool symbols, uint8_t focus);
  static void drawPWGenResult(const char* pw, uint16_t scrollX);

  // ── Settings ──────────────────────────────────────────────
  static void drawSettings(const char* items[], uint8_t count,
                            uint8_t sel, uint8_t scroll);
  static void drawAbout(const char* version, uint8_t batPct);

  // ── Notification overlays ─────────────────────────────────
  static void drawMessage(const char* title, const char* body,
                           bool isError = false);
  static void drawProgress(const char* label, uint8_t pct);
  static void drawBatteryLow(uint8_t pct);

  // Raw U8g2 object — accessible for special cases
  static U8G2_SH1107_64X128_F_HW_I2C u8g2;

private:
  static void   _statusBar(uint8_t batPct, bool charging,
                            bool ble, bool locked);
  static void   _drawBatIcon(uint8_t x, uint8_t y, uint8_t pct,
                              bool charging);
  static void   _drawScrollbar(uint8_t total, uint8_t visible,
                                uint8_t topIdx);
  static bool   _asleep;
};

// ── CharInput screen (added) ──────────────────────────────────
#include "../app/CharInput.h"   // for CharInputState

// Separate declaration block appended to Display class
// (in practice, merge into class body in Display.h)
// Added functions:
//   drawCharInput(state)  — main text-entry screen
//   drawAdminSetup(companyBuf, adminBuf, activeField)
