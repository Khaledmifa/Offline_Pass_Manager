#pragma once
#include <Arduino.h>
#include "../hal/Joystick.h"
#include "../core/SiteDatabase.h"

// =============================================================
//  CharInput.h | Joystick character-by-character text entry
//
//  Usage:
//    CharInput::begin("Site name", 32, true);   // with autocomplete
//    while(true) {
//      JoyEvent e = Joystick::poll();
//      if (CharInput::feed(e)) break;            // done
//      Display::drawCharInput(CharInput::state());
//    }
//    String result = CharInput::result();
//
//  Charset cycles:  A-Z  a-z  0-9  . @ - _ ! # & ( ) / space
//  UP   → next char in charset
//  DOWN → prev char in charset
//  RIGHT / PRESS → confirm current char, advance position
//  LEFT (short)  → backspace: remove last confirmed char
//  HOLD RIGHT (>800ms) → finish and return full string
// =============================================================

struct CharInputState {
  char     buf[64];          // confirmed characters so far
  uint8_t  bufLen;           // confirmed length
  char     pendingChar;      // character currently being cycled
  uint8_t  charsetIdx;       // index into CHARSET

  // Autocomplete
  const char* suggestions[5];
  uint8_t     suggCount;
  uint8_t     suggSel;       // which suggestion is highlighted (-1 = none)
  bool        inSuggestMode; // navigating suggestions vs typing

  const char* prompt;
};

class CharInput {
public:
  // begin() resets all state
  static void   begin(const char* prompt, uint8_t maxLen,
                      bool enableSuggestions = false);

  // feed() processes one joystick event.
  // Returns true when the user has confirmed the full string.
  static bool   feed(JoyEvent e);

  // Access current state (for rendering)
  static const CharInputState& state();

  // Result after feed() returns true
  static String result();

  // Full charset available to user
  static const char* CHARSET;
  static const uint8_t CHARSET_LEN;

private:
  static CharInputState _s;
  static uint8_t        _maxLen;
  static bool           _enableSug;
  static uint32_t       _rightHeld;

  static void   _updateSuggestions();
  static void   _confirmPendingChar();
  static void   _backspace();
  static void   _acceptSuggestion(uint8_t idx);
};
