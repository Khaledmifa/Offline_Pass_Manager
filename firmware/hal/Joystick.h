#pragma once
#include <Arduino.h>
// =============================================================
//  Joystick.h | XY analogue joystick + push-button driver
//  Returns a typed JoyEvent per poll() call.
// =============================================================
enum class JoyEvent : uint8_t { NONE, UP, DOWN, LEFT, RIGHT, PRESS };

class Joystick {
public:
  static void     begin();
  static JoyEvent poll();
private:
  static bool     _btnLast;
  static uint32_t _btnTime;
  static JoyEvent _dirLast;
  static uint32_t _repeatTime;
};
