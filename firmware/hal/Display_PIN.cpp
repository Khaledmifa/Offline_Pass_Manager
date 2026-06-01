// =============================================================
//  Display_PIN.cpp — Alphanumeric PIN entry screen
//  Merged into Display.cpp in the final build.
//  Shows: typed text (masked as *), strength indicators,
//         current pending character, and joystick hint.
// =============================================================
#include "Display.h"
#include "../app/CharInput.h"
#include "../config.h"

// Strength indicator layout:
//   [A-Z] [a-z] [0-9] [!] ← filled when requirement met
//   ●     ○     ○     ○   ← filled dot = satisfied

void Display::drawPINEntryAlpha(const CharInputState& s,
                                  bool isConfirm,
                                  bool wrongMsg,
                                  bool strengthOk) {
  u8g2.clearBuffer();

  // ── Title ────────────────────────────────────────────────
  u8g2.setFont(FONT_BOLD);
  u8g2.drawStr(2, TITLE_Y, isConfirm ? "Confirm PIN" : "Set Device PIN");
  u8g2.drawHLine(0, SEP_Y, OLED_W);

  if (wrongMsg) {
    u8g2.setFont(FONT_NORMAL);
    u8g2.drawStr(4, ITEM_Y(1), "PIN does not match!");
    u8g2.sendBuffer();
    return;
  }

  // ── Masked input line: show * for each confirmed char ────
  char masked[PIN_MAX_LEN + 4] = {};
  uint8_t blen = s.bufLen;
  for (uint8_t i = 0; i < blen && i < 18; i++) masked[i] = '*';
  // Append pending char indicator
  char line[32];
  snprintf(line, sizeof(line), "%s[%c]", masked, s.pendingChar);
  u8g2.setFont(FONT_NORMAL);
  u8g2.drawStr(2, ITEM_Y(0), line);

  // ── Character count badge ─────────────────────────────────
  char cnt[12]; snprintf(cnt, sizeof(cnt), "%u/%u", blen, PIN_MIN_LEN);
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(OLED_W - 26, ITEM_Y(0), cnt);

  // ── Strength requirements (live feedback) ─────────────────
  // Analyse buffer so far
  bool hasUpper  = false, hasLower = false;
  bool hasDigit  = false, hasSym   = false;
  for (uint8_t i = 0; i < blen; i++) {
    char c = s.buf[i];
    if (c >= 'A' && c <= 'Z') hasUpper = true;
    else if (c >= 'a' && c <= 'z') hasLower = true;
    else if (c >= '0' && c <= '9') hasDigit = true;
    else                            hasSym   = true;
  }

  struct Req { const char* lbl; bool met; };
  Req reqs[] = {{"A-Z",hasUpper},{"a-z",hasLower},{"0-9",hasDigit},{"!#@",hasSym}};
  uint8_t rx = 2;
  for (uint8_t i = 0; i < 4; i++) {
    u8g2.setFont(FONT_TINY);
    if (reqs[i].met) u8g2.drawDisc(rx + 3, ITEM_Y(2), 3);
    else             u8g2.drawCircle(rx + 3, ITEM_Y(2), 3);
    u8g2.drawStr(rx, ITEM_Y(2) + 9, reqs[i].lbl);
    rx += 30;
  }

  // ── Length check ─────────────────────────────────────────
  u8g2.setFont(FONT_TINY);
  if (blen >= PIN_MIN_LEN) u8g2.drawDisc(OLED_W - 18, ITEM_Y(2), 3);
  else                     u8g2.drawCircle(OLED_W - 18, ITEM_Y(2), 3);
  u8g2.drawStr(OLED_W - 14, ITEM_Y(2) + 9, "8+");

  // ── Bottom hint ───────────────────────────────────────────
  u8g2.setFont(FONT_TINY);
  u8g2.drawStr(0, 63, "\x18\x19 char  \x1a confirm  \x1b del  hold\x1a done");

  u8g2.sendBuffer();
}
