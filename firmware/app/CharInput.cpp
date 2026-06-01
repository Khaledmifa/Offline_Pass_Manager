#include "CharInput.h"
#include "../config.h"
#include <string.h>

// Charset:  A-Z (26) + a-z (26) + 0-9 (10) + punctuation (12) = 74
const char* CharInput::CHARSET =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 .@-_!#&()/";
const uint8_t CharInput::CHARSET_LEN = 74;

CharInputState CharInput::_s         = {};
uint8_t        CharInput::_maxLen    = 32;
bool           CharInput::_enableSug = false;
uint32_t       CharInput::_rightHeld = 0;

void CharInput::begin(const char* prompt, uint8_t maxLen, bool enableSug) {
  memset(&_s, 0, sizeof(_s));
  _s.prompt        = prompt;
  _s.charsetIdx    = 0;
  _s.pendingChar   = CHARSET[0];   // 'A' by default
  _s.inSuggestMode = false;
  _s.suggSel       = 0xFF;         // nothing selected
  _maxLen          = maxLen;
  _enableSug       = enableSug;
  _rightHeld       = 0;
}

const CharInputState& CharInput::state() { return _s; }

String CharInput::result() {
  return String(_s.buf);
}

void CharInput::_updateSuggestions() {
  if (!_enableSug || _s.bufLen == 0) { _s.suggCount = 0; return; }
  _s.suggCount = SiteDatabase::search(_s.buf, _s.suggestions, 5);
}

void CharInput::_confirmPendingChar() {
  if (_s.bufLen >= _maxLen - 1) return;
  _s.buf[_s.bufLen++] = _s.pendingChar;
  _s.buf[_s.bufLen]   = '\0';
  _s.charsetIdx  = 0;           // reset to 'A' for next char
  _s.pendingChar = CHARSET[0];
  if (_enableSug) _updateSuggestions();
}

void CharInput::_backspace() {
  if (_s.bufLen == 0) return;
  _s.buf[--_s.bufLen] = '\0';
  _s.charsetIdx  = 0;
  _s.pendingChar = CHARSET[0];
  if (_enableSug) _updateSuggestions();
}

void CharInput::_acceptSuggestion(uint8_t idx) {
  if (idx >= _s.suggCount) return;
  const char* sug = _s.suggestions[idx];
  uint8_t len = strlen(sug);
  if (len >= _maxLen) len = _maxLen - 1;
  strncpy(_s.buf, sug, len);
  _s.buf[len]   = '\0';
  _s.bufLen     = len;
  _s.charsetIdx = 0;
  _s.pendingChar = CHARSET[0];
  _s.inSuggestMode = false;
  _s.suggSel = 0xFF;
  _updateSuggestions();
}

bool CharInput::feed(JoyEvent e) {
  // ── Suggestion navigation mode ────────────────────────────
  if (_s.inSuggestMode && _s.suggCount > 0) {
    if (e == JoyEvent::UP) {
      // Exit suggestion mode back to typing
      _s.inSuggestMode = false;
      _s.suggSel = 0xFF;
      return false;
    }
    if (e == JoyEvent::DOWN) {
      if (_s.suggSel == 0xFF) _s.suggSel = 0;
      else _s.suggSel = (_s.suggSel + 1) % _s.suggCount;
      return false;
    }
    if (e == JoyEvent::PRESS || e == JoyEvent::RIGHT) {
      if (_s.suggSel != 0xFF) {
        _acceptSuggestion(_s.suggSel);
        return true;   // accepted suggestion = done
      }
    }
    if (e == JoyEvent::LEFT) {
      _s.inSuggestMode = false;
      _s.suggSel = 0xFF;
      return false;
    }
    return false;
  }

  // ── Character cycling mode ────────────────────────────────
  if (e == JoyEvent::UP) {
    _s.charsetIdx = (_s.charsetIdx + 1) % CHARSET_LEN;
    _s.pendingChar = CHARSET[_s.charsetIdx];
  }

  if (e == JoyEvent::DOWN) {
    _s.charsetIdx = (_s.charsetIdx == 0) ? CHARSET_LEN - 1 : _s.charsetIdx - 1;
    _s.pendingChar = CHARSET[_s.charsetIdx];
  }

  if (e == JoyEvent::RIGHT || e == JoyEvent::PRESS) {
    // Track hold time
    if (e == JoyEvent::RIGHT) {
      if (_rightHeld == 0) _rightHeld = millis();
      if (millis() - _rightHeld > 800) {
        // Long hold RIGHT = finish
        if (_s.pendingChar != CHARSET[0] || _s.bufLen > 0) {
          // Don't add a trailing 'A' — only finish
        }
        return (_s.bufLen > 0);
      }
    } else {
      _rightHeld = 0;
    }

    // Short press: confirm current pending char
    _confirmPendingChar();

    // Enter suggest mode if suggestions available
    if (_enableSug && _s.suggCount > 0 && _s.bufLen >= 1) {
      _s.inSuggestMode = true;
      _s.suggSel = 0;
    }
  } else {
    _rightHeld = 0;
  }

  if (e == JoyEvent::LEFT) {
    if (_s.bufLen > 0) {
      _backspace();
    } else {
      return false;   // nothing to delete; caller may cancel
    }
  }

  return false;   // still editing
}
