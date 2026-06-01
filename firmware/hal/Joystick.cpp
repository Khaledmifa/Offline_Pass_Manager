#include "Joystick.h"
#include "../config.h"

bool     Joystick::_btnLast    = false;
uint32_t Joystick::_btnTime    = 0;
JoyEvent Joystick::_dirLast    = JoyEvent::NONE;
uint32_t Joystick::_repeatTime = 0;

void Joystick::begin() {
  pinMode(JOY_BTN_PIN, INPUT_PULLUP);
  analogReadResolution(12);
}

JoyEvent Joystick::poll() {
  const uint32_t now = millis();

  // Button (active-LOW via INPUT_PULLUP)
  bool btnDown = !digitalRead(JOY_BTN_PIN);
  if (btnDown && !_btnLast) { _btnLast = true;  _btnTime = now; }
  else if (!btnDown && _btnLast) {
    _btnLast = false;
    if (now - _btnTime < 600) return JoyEvent::PRESS;
  }

  // Axes
  int dx = analogRead(JOY_X_PIN) - JOY_CENTRE;
  int dy = analogRead(JOY_Y_PIN) - JOY_CENTRE;

  JoyEvent dir = JoyEvent::NONE;
  if (abs(dx) > abs(dy)) {
    if      (dx >  JOY_DEADZONE) dir = JoyEvent::RIGHT;
    else if (dx < -JOY_DEADZONE) dir = JoyEvent::LEFT;
  } else {
    if      (dy >  JOY_DEADZONE) dir = JoyEvent::DOWN;
    else if (dy < -JOY_DEADZONE) dir = JoyEvent::UP;
  }

  // Repeat logic for held direction
  if (dir != JoyEvent::NONE) {
    if (dir != _dirLast) { _dirLast = dir; _repeatTime = now; return dir; }
    if (now - _repeatTime >= JOY_REPEAT_MS) { _repeatTime = now; return dir; }
  } else {
    _dirLast = JoyEvent::NONE;
  }
  return JoyEvent::NONE;
}
