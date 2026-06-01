#include "Display.h"
#include "../config.h"
#include <Wire.h>

// Constructor: SH1107 64x128, rotated 90° → 128x64 landscape
U8G2_SH1107_64X128_F_HW_I2C Display::u8g2(U8G2_R1, U8X8_PIN_NONE);
bool Display::_asleep = false;

// ── Font helpers ──────────────────────────────────────────────
#define FONT_TINY    u8g2_font_4x6_tf
#define FONT_SMALL   u8g2_font_5x8_tf
#define FONT_NORMAL  u8g2_font_6x10_tf
#define FONT_BOLD    u8g2_font_7x13B_tf
#define FONT_ARABIC  u8g2_font_unifont_t_arabic
#define FONT_CJK     u8g2_font_wqy12_t_gb2312b

#define LINE_H       11   // pixel height per content line
#define ITEM_Y(n)    (CONTENT_Y + 2 + (n) * LINE_H)
#define TITLE_Y      (CONTENT_Y + 11)
#define SEP_Y        (CONTENT_Y + 13)

void Display::begin() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  u8g2.begin();
  u8g2.setContrast(220);
  _asleep = false;
}

void Display::sleep() {
  u8g2.setPowerSave(1);
  _asleep = true;
}

void Display::wake() {
  u8g2.setPowerSave(0);
  _asleep = false;
}

// ══════════════════════════════════════════════════════════════
//  PRIVATE HELPERS
// ══════════════════════════════════════════════════════════════
void Display::_drawBatIcon(uint8_t x, uint8_t y, uint8_t pct, bool charging) {
  // Outer shell: 18×9
  u8g2.drawFrame(x, y, 18, 9);
  // Nub
  u8g2.drawBox(x + 18, y + 3, 2, 3);
  // Fill
  uint8_t fill = (uint8_t)(pct * 15 / 100);
  if (fill > 0) u8g2.drawBox(x + 2, y + 2, fill, 5);
  // Charging flash
  if (charging) {
    u8g2.setFont(FONT_TINY);
    u8g2.drawStr(x + 6, y + 7, "+");
  }
}

void Display::_drawScrollbar(uint8_t total, uint8_t visible, uint8_t topIdx) {
  if (total <= visible) return;
  const uint8_t X     = OLED_W - 4;
  const uint8_t H     = CONTENT_H - 14;
  u8g2.drawVLine(X, CONTENT_Y + 14, H);
  uint8_t barH  = max(4, (int)(H * visible / total));
  uint8_t barY  = CONTENT_Y + 14 + (H - barH) * topIdx / (total - visible);
  u8g2.drawBox(X, barY, 3, barH);
}

void Display::_statusBar(uint8_t batPct, bool charging, bool ble, bool locked) {
  u8g2.setFont(FONT_TINY);
  // Battery icon left
  _drawBatIcon(2, 1, batPct, charging);
  // Percent text
  char buf[5]; snprintf(buf, sizeof(buf), "%3u%%", batPct);
  u8g2.drawStr(23, 9, buf);
  // HID mode badge
  u8g2.drawStr(58, 9, ble ? "BLE" : "USB");
  // Device name centre
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(85, 9, "[PM]");
  // Lock icon right
  if (locked) u8g2.drawStr(116, 9, "L");
  // Separator line
  u8g2.drawHLine(0, STATUSBAR_H, OLED_W);
}

// ══════════════════════════════════════════════════════════════
//  SETUP / WIZARD SCREENS
// ══════════════════════════════════════════════════════════════
void Display::drawBootSplash(const char* version) {
  u8g2.clearBuffer();
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(24, 28, "PASSWORD");
  u8g2.drawStr(22, 42, "MANAGER");
  u8g2.drawHLine(10, 46, 108);
  u8g2.setFont(FONT_SMALL);
  u8g2.drawStr(34, 58, "ESP32-S3");
  u8g2.setFont(FONT_TINY);
  char buf[20]; snprintf(buf, sizeof(buf), "v%s", version);
  u8g2.drawStr(100, 62, buf);
  u8g2.sendBuffer();
}

void Display::drawSetupAP(const char* ssid, const char* ip) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "WiFi Setup");
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  u8g2.setFont(FONT_SMALL);
  u8g2.drawStr(2, ITEM_Y(1), "Connect to:");
  u8g2.setFont(FONT_NORMAL);
  u8g2.drawStr(2, ITEM_Y(2), ssid);
  u8g2.setFont(FONT_SMALL);
  u8g2.drawStr(2, ITEM_Y(3), "Then open:");
  u8g2.setFont(FONT_NORMAL);
  u8g2.drawStr(2, ITEM_Y(4), ip);
  u8g2.sendBuffer();
}

void Display::drawWizardProgress(uint8_t step, uint8_t total) {
  // Draw 5px progress bar across status bar area bottom
  u8g2.drawHLine(0, STATUSBAR_H - 1, (OLED_W * step / total));
}

void Display::drawLanguageSelect(const char* langs[], uint8_t count,
                                  uint8_t sel, uint8_t scroll) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "Language");
  u8g2.drawHLine(0, SEP_Y, OLED_W - 6);
  uint8_t visible = 4;
  for (uint8_t i = 0; i < visible && (scroll + i) < count; i++) {
    uint8_t idx = scroll + i;
    uint8_t y   = ITEM_Y(i);
    if (idx == sel) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - 9, OLED_W - 6, 11);
      u8g2.setDrawColor(0);
    } else {
      u8g2.setDrawColor(1);
    }
    u8g2.setFont(FONT_NORMAL);
    u8g2.drawStr(4, y, langs[idx]);
    u8g2.setDrawColor(1);
  }
  _drawScrollbar(count, visible, scroll);
  u8g2.sendBuffer();
}

void Display::drawPINEntry(const uint8_t* digits, uint8_t len,
                            uint8_t active, bool isConfirm, bool isWrong) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, isConfirm ? "Confirm PIN" : "Set PIN");
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  if (isWrong) {
    u8g2.setFont(FONT_SMALL);
    u8g2.drawStr(20, ITEM_Y(1), "PIN mismatch!");
    u8g2.sendBuffer(); return;
  }
  // Draw digit boxes centred
  const uint8_t BOX_W = 18, BOX_H = 18, GAP = 6;
  uint8_t total_w = len * BOX_W + (len - 1) * GAP;
  uint8_t startX  = (OLED_W - total_w) / 2;
  uint8_t boxY    = 36;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t bx = startX + i * (BOX_W + GAP);
    u8g2.drawFrame(bx, boxY, BOX_W, BOX_H);
    if (i == active) u8g2.drawFrame(bx - 1, boxY - 1, BOX_W + 2, BOX_H + 2);
    if (digits[i] != 0xFF) {
      u8g2.setFont(FONT_BOLD);
      char c = isConfirm ? '*' : ('0' + digits[i]);
      char s[2] = {c, 0};
      u8g2.drawStr(bx + 5, boxY + 13, s);
    }
  }
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(10, 62, "\x18\x19 change  \x1a confirm");
  u8g2.sendBuffer();
}

void Display::drawFPEnroll(uint8_t scanStep, uint8_t fpIndex) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "Fingerprint");
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  u8g2.setFont(FONT_NORMAL);
  if (scanStep == 1) u8g2.drawStr(2, ITEM_Y(1), "Place finger");
  else               u8g2.drawStr(2, ITEM_Y(1), "Lift & replace");
  // Progress dots (5 fp slots)
  uint8_t dotX = 4;
  for (uint8_t i = 0; i < FP_MAX_SLOTS; i++) {
    if (i < fpIndex) u8g2.drawDisc(dotX, 56, 4);
    else              u8g2.drawCircle(dotX, 56, 4);
    dotX += 12;
  }
  // Scan step indicator
  u8g2.setFont(FONT_TINY);
  char buf[16]; snprintf(buf, sizeof(buf), "Scan %u/2  FP#%u", scanStep, fpIndex + 1);
  u8g2.drawStr(70, 60, buf);
  u8g2.sendBuffer();
}

void Display::drawFPAnother(uint8_t enrolled) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "FP Saved!");
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  u8g2.setFont(FONT_NORMAL);
  char buf[28]; snprintf(buf, sizeof(buf), "%u enrolled, add more?", enrolled);
  u8g2.drawStr(2, ITEM_Y(1), buf);
  // YES / NO
  u8g2.drawBox(4,  48, 38, 13);
  u8g2.setDrawColor(0); u8g2.drawStr(10, 58, "YES");
  u8g2.setDrawColor(1); u8g2.drawFrame(50, 48, 38, 13);
  u8g2.drawStr(58, 58, "NO");
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(95, 58, "\x1b back");
  u8g2.sendBuffer();
}

void Display::drawBackupFreq(uint8_t sel) {
  static const char* opts[] = {"Every day","Every 3 days","Every week","2 weeks","Monthly"};
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "Backup Freq.");
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  for (uint8_t i = 0; i < 5; i++) {
    uint8_t y = ITEM_Y(i);
    if (i == sel) {
      u8g2.drawBox(0, y - 9, OLED_W - 4, 11);
      u8g2.setDrawColor(0);
    }
    u8g2.setFont(FONT_NORMAL);
    u8g2.drawStr(4, y, opts[i]);
    u8g2.setDrawColor(1);
  }
  u8g2.sendBuffer();
}

void Display::drawWiFiConnecting(const char* ssid) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "Connecting...");
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  u8g2.setFont(FONT_NORMAL);
  u8g2.drawStr(2, ITEM_Y(1), ssid);
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(2, 62, "Please wait");
  u8g2.sendBuffer();
}

void Display::drawSetupDone() {
  u8g2.clearBuffer();
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(10, 30, "Setup Complete!");
  u8g2.drawHLine(8, 33, 112);
  u8g2.setFont(FONT_NORMAL);
  u8g2.drawStr(14, 46, "Touch sensor");
  u8g2.drawStr(20, 58, "to unlock");
  u8g2.sendBuffer();
}

// ══════════════════════════════════════════════════════════════
//  LOCK SCREENS
// ══════════════════════════════════════════════════════════════
void Display::drawLocked(uint8_t batPct, bool charging) {
  u8g2.clearBuffer();
  _statusBar(batPct, charging, false, true);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(36, 32, "LOCKED");
  u8g2.drawHLine(8, 34, 112);
  u8g2.setFont(FONT_SMALL);
  u8g2.drawStr(10, 46, "Touch sensor: fingerprint");
  u8g2.drawStr(10, 58, "Press button: PIN");
  u8g2.sendBuffer();
}

void Display::drawUnlockFP(bool scanning, bool failed) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, true);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "Fingerprint");
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  u8g2.setFont(FONT_NORMAL);
  if (failed) {
    u8g2.drawStr(10, ITEM_Y(2), "No match. Try again");
  } else if (scanning) {
    u8g2.drawStr(20, ITEM_Y(1), "Scanning...");
    // Animated dots — caller redraws each tick
    static uint8_t tick = 0; tick++;
    for (uint8_t i = 0; i < 3; i++)
      if (tick % 8 > i * 2) u8g2.drawDisc(48 + i * 12, 52, 3);
      else u8g2.drawCircle(48 + i * 12, 52, 3);
  } else {
    u8g2.drawStr(20, ITEM_Y(2), "Place finger");
  }
  u8g2.sendBuffer();
}

void Display::drawUnlockPIN(const uint8_t* digits, uint8_t len,
                              uint8_t active, uint8_t wrongCount) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, true);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "Enter PIN");
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  if (wrongCount > 0) {
    u8g2.setFont(FONT_SMALL);
    char w[24]; snprintf(w, sizeof(w), "Wrong! %u/%u attempts", wrongCount, PIN_MAX_ATTEMPTS);
    u8g2.drawStr(2, ITEM_Y(1), w);
  }
  const uint8_t BOX_W = 16, GAP = 4;
  uint8_t total_w = len * BOX_W + (len - 1) * GAP;
  uint8_t sx = (OLED_W - total_w) / 2;
  uint8_t by = 38;
  for (uint8_t i = 0; i < len; i++) {
    uint8_t bx = sx + i * (BOX_W + GAP);
    u8g2.drawFrame(bx, by, BOX_W, 16);
    if (i == active) u8g2.drawFrame(bx-1, by-1, BOX_W+2, 18);
    if (digits[i] != 0xFF) {
      u8g2.setFont(FONT_BOLD);
      u8g2.drawStr(bx + 4, by + 12, "*");
    }
  }
  u8g2.sendBuffer();
}

// ══════════════════════════════════════════════════════════════
//  MAIN MENU
// ══════════════════════════════════════════════════════════════
void Display::drawMainMenu(const char* items[], uint8_t count, uint8_t sel,
                            uint8_t scroll, uint8_t batPct, bool ble) {
  u8g2.clearBuffer();
  _statusBar(batPct, false, ble, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "Main Menu");
  u8g2.drawHLine(0, SEP_Y, OLED_W - 6);
  uint8_t visible = 4;
  for (uint8_t i = 0; i < visible && (scroll + i) < count; i++) {
    uint8_t idx = scroll + i;
    uint8_t y   = ITEM_Y(i);
    if (idx == sel) {
      u8g2.drawBox(0, y - 9, OLED_W - 6, 11);
      u8g2.setDrawColor(0);
    }
    u8g2.setFont(FONT_NORMAL);
    u8g2.drawStr(4, y, items[idx]);
    u8g2.setDrawColor(1);
  }
  _drawScrollbar(count, visible, scroll);
  u8g2.sendBuffer();
}

// ══════════════════════════════════════════════════════════════
//  CREDENTIALS
// ══════════════════════════════════════════════════════════════
void Display::drawCredList(const char* names[], uint8_t count,
                            uint8_t sel, uint8_t scroll) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "Credentials");
  u8g2.drawHLine(0, SEP_Y, OLED_W - 6);
  uint8_t visible = 4;
  for (uint8_t i = 0; i < visible && (scroll + i) < count; i++) {
    uint8_t idx = scroll + i;
    uint8_t y   = ITEM_Y(i);
    if (idx == sel) {
      u8g2.drawBox(0, y - 9, OLED_W - 6, 11);
      u8g2.setDrawColor(0);
    }
    u8g2.setFont(FONT_NORMAL);
    // Truncate to 18 chars
    char buf[20]; strncpy(buf, names[idx], 18); buf[18] = 0;
    u8g2.drawStr(4, y, buf);
    u8g2.setDrawColor(1);
  }
  _drawScrollbar(count, visible, scroll);
  u8g2.sendBuffer();
}

void Display::drawCredDetail(const char* name, const char* user) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  char title[20]; strncpy(title, name, 18); title[18] = 0;
  u8g2.drawStr(2, TITLE_Y, title);
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  u8g2.setFont(FONT_SMALL);
  u8g2.drawStr(2, ITEM_Y(1), "User:");
  u8g2.setFont(FONT_NORMAL);
  char ubuf[20]; strncpy(ubuf, user, 18); ubuf[18] = 0;
  u8g2.drawStr(2, ITEM_Y(2), ubuf);
  u8g2.setFont(FONT_SMALL);
  u8g2.drawStr(2, ITEM_Y(3), "Pass: ************");
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(2, 63, "\x1a send  \x1b back");
  u8g2.sendBuffer();
}

void Display::drawSendMenu(uint8_t sel) {
  static const char* opts[] = {"User + Password", "Username only", "Password only"};
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "Send via HID");
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  for (uint8_t i = 0; i < 3; i++) {
    uint8_t y = ITEM_Y(i);
    if (i == sel) {
      u8g2.drawBox(0, y - 9, OLED_W, 11);
      u8g2.setDrawColor(0);
    }
    u8g2.setFont(FONT_NORMAL);
    u8g2.drawStr(4, y, opts[i]);
    u8g2.setDrawColor(1);
  }
  u8g2.sendBuffer();
}

void Display::drawSending(const char* label) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "Typing...");
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  u8g2.setFont(FONT_NORMAL);
  u8g2.drawStr(2, ITEM_Y(2), label);
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(2, 63, "Keep focus on login field");
  u8g2.sendBuffer();
}

// ══════════════════════════════════════════════════════════════
//  PASSWORD GENERATOR
// ══════════════════════════════════════════════════════════════
void Display::drawPWGenOptions(uint8_t len, bool upper, bool lower,
                                bool digs, bool syms, uint8_t focus) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "PW Generator");
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  // Length row
  char lbuf[20]; snprintf(lbuf, sizeof(lbuf), "Length: %2u", len);
  if (focus == 0) { u8g2.drawBox(0, ITEM_Y(0)-9, OLED_W, 11); u8g2.setDrawColor(0); }
  u8g2.setFont(FONT_NORMAL); u8g2.drawStr(4, ITEM_Y(0), lbuf); u8g2.setDrawColor(1);
  // Charset checkboxes
  struct { const char* lbl; bool on; } opts[4] = {
    {"[A-Z]", upper}, {"[a-z]", lower}, {"[0-9]", digs}, {"[!@#]", syms}
  };
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t cx = 4 + i * 30, cy = ITEM_Y(2);
    if (focus == i + 1) u8g2.drawFrame(cx - 2, cy - 9, 28, 11);
    u8g2.setFont(FONT_TINY);
    u8g2.drawStr(cx, cy, opts[i].on ? "[X]" : "[ ]");
    u8g2.drawStr(cx, cy + 8, opts[i].lbl);
  }
  u8g2.sendBuffer();
}

void Display::drawPWGenResult(const char* pw, uint16_t scrollX) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "Generated PW");
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  u8g2.setFont(FONT_SMALL);
  // Scrolling horizontal text
  u8g2.drawStr(2 - (int)scrollX, ITEM_Y(2), pw);
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(2, 63, "\x1a save  \x1b regen  hold: copy");
  u8g2.sendBuffer();
}

// ══════════════════════════════════════════════════════════════
//  SETTINGS
// ══════════════════════════════════════════════════════════════
void Display::drawSettings(const char* items[], uint8_t count,
                             uint8_t sel, uint8_t scroll) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "Settings");
  u8g2.drawHLine(0, SEP_Y, OLED_W - 6);
  uint8_t visible = 4;
  for (uint8_t i = 0; i < visible && (scroll + i) < count; i++) {
    uint8_t idx = scroll + i;
    uint8_t y   = ITEM_Y(i);
    if (idx == sel) {
      u8g2.drawBox(0, y - 9, OLED_W - 6, 11);
      u8g2.setDrawColor(0);
    }
    u8g2.setFont(FONT_NORMAL);
    u8g2.drawStr(4, y, items[idx]);
    u8g2.setDrawColor(1);
  }
  _drawScrollbar(count, visible, scroll);
  u8g2.sendBuffer();
}

void Display::drawAbout(const char* version, uint8_t batPct) {
  u8g2.clearBuffer();
  _statusBar(batPct, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "About");
  u8g2.drawHLine(0, SEP_Y, OLED_W);
  u8g2.setFont(FONT_SMALL);
  u8g2.drawStr(2, ITEM_Y(1), "HW Password Manager");
  char buf[20]; snprintf(buf, sizeof(buf), "Firmware v%s", version);
  u8g2.drawStr(2, ITEM_Y(2), buf);
  snprintf(buf, sizeof(buf), "Battery: %u%%", batPct);
  u8g2.drawStr(2, ITEM_Y(3), buf);
  u8g2.sendBuffer();
}

// ══════════════════════════════════════════════════════════════
//  OVERLAYS
// ══════════════════════════════════════════════════════════════
void Display::drawMessage(const char* title, const char* body, bool isError) {
  u8g2.clearBuffer();
  u8g2.drawFrame(4, 8, OLED_W - 8, OLED_H - 10);
  if (isError) u8g2.drawFrame(5, 9, OLED_W - 10, OLED_H - 12);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(10, 22, title);
  u8g2.drawHLine(8, 25, OLED_W - 16);
  u8g2.setFont(FONT_SMALL);
  u8g2.drawStr(8, 38, body);
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(44, 56, "OK: press");
  u8g2.sendBuffer();
}

void Display::drawProgress(const char* label, uint8_t pct) {
  u8g2.clearBuffer();
  u8g2.setFont(FONT_NORMAL);
  u8g2.drawStr(4, 28, label);
  u8g2.drawFrame(4, 36, OLED_W - 8, 10);
  u8g2.drawBox(5, 37, (OLED_W - 10) * pct / 100, 8);
  char buf[8]; snprintf(buf, sizeof(buf), "%u%%", pct);
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(58, 56, buf);
  u8g2.sendBuffer();
}

void Display::drawBatteryLow(uint8_t pct) {
  u8g2.clearBuffer();
  u8g2.drawFrame(4, 8, OLED_W - 8, OLED_H - 10);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(14, 28, "Battery Low!");
  char buf[12]; snprintf(buf, sizeof(buf), "%u%% remaining", pct);
  u8g2.setFont(FONT_SMALL);
  u8g2.drawStr(10, 42, buf);
  u8g2.drawStr(10, 54, "Please charge soon");
  u8g2.sendBuffer();
}

// ═══════════════════════════════════════════════════════════════
//  CHARACTER INPUT SCREEN
//  Shows: [prompt] | [typed + cursor] | [pending char] |
//         [up to 3 suggestions]
// ═══════════════════════════════════════════════════════════════
void Display::drawCharInput(const CharInputState& s) {
  u8g2.clearBuffer();

  // Title / prompt
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, s.prompt);
  u8g2.drawHLine(0, SEP_Y, OLED_W);

  // ── Typed buffer + pending char + cursor ──────────────────
  // Show last 16 chars of buffer so it doesn't overflow
  char preview[32];
  const char* buf = s.buf;
  uint8_t blen = s.bufLen;
  if (blen > 14) buf += (blen - 14);   // scroll left if too long
  snprintf(preview, sizeof(preview), "%s%c_", buf, s.pendingChar);

  u8g2.setFont(FONT_NORMAL);
  u8g2.drawStr(2, ITEM_Y(0), preview);

  // ── Charset navigation arrows ─────────────────────────────
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(2, ITEM_Y(1) - 2, "\x18 next char");
  u8g2.drawStr(64, ITEM_Y(1) - 2, "\x19 prev");

  // ── Autocomplete suggestions ──────────────────────────────
  if (s.suggCount > 0) {
    u8g2.drawHLine(0, ITEM_Y(1) + 2, OLED_W);
    uint8_t maxShow = min(s.suggCount, (uint8_t)3);
    for (uint8_t i = 0; i < maxShow; i++) {
      uint8_t sy = ITEM_Y(2) + i * 10;
      if (s.inSuggestMode && s.suggSel == i) {
        u8g2.drawBox(0, sy - 8, OLED_W, 10);
        u8g2.setDrawColor(0);
      }
      u8g2.setFont(FONT_SMALL);
      // Truncate suggestion to fit
      char tmp[20]; strncpy(tmp, s.suggestions[i], 18); tmp[18] = 0;
      u8g2.drawStr(3, sy, tmp);
      u8g2.setDrawColor(1);
    }
  }

  // ── Bottom hint ───────────────────────────────────────────
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(0, 63,
    s.inSuggestMode ? "\x1a accept  \x1b cancel" : "\x1a confirm  \x1b del  hold\x1a done");

  u8g2.sendBuffer();
}

// ── Admin setup screen (company + admin email) ────────────────
void Display::drawAdminSetup(const char* company, const char* adminEmail,
                               uint8_t activeField) {
  u8g2.clearBuffer();
  _statusBar(0, false, false, false);
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, "Device Settings");
  u8g2.drawHLine(0, SEP_Y, OLED_W);

  // Field 0: Company name
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(2, ITEM_Y(0) - 2, "Company (optional)");
  if (activeField == 0) u8g2.drawFrame(0, ITEM_Y(0), OLED_W, 11);
  u8g2.setFont(FONT_SMALL);
  char cbuf[22]; strncpy(cbuf, company[0] ? company : "---", 20); cbuf[20] = 0;
  u8g2.drawStr(4, ITEM_Y(0) + 9, cbuf);

  // Field 1: Admin email
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(2, ITEM_Y(2) - 1, "Admin email");
  if (activeField == 1) u8g2.drawFrame(0, ITEM_Y(2), OLED_W, 11);
  u8g2.setFont(FONT_SMALL);
  char ebuf[22]; strncpy(ebuf, adminEmail[0] ? adminEmail : "---", 20); ebuf[20] = 0;
  u8g2.drawStr(4, ITEM_Y(2) + 9, ebuf);

  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(2, 63, "\x1a edit  \x18\x19 field  hold\x1a done");
  u8g2.sendBuffer();
}
