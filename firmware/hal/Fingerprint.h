#pragma once
#include <Arduino.h>
#include <Adafruit_Fingerprint.h>

// =============================================================
//  Fingerprint.h | GROW R503 driver via UART2
//  Stores up to FP_MAX_SLOTS templates on sensor flash.
//  Hardware password protects against sensor spoofing.
// =============================================================

enum class FPResult : uint8_t {
  OK, FAIL, NO_FINGER, TIMEOUT, TAMPERED, ENROLL_STEP1_DONE
};

struct FPSlotMap {
  uint8_t count = 0;
  bool    used[FP_MAX_SLOTS] = {};
  char    label[FP_MAX_SLOTS][16] = {};
};

class Fingerprint {
public:
  static bool     begin();        // init UART2 + verify hw password
  static bool     isReady();

  // Verification
  static FPResult verify(uint8_t& matchedSlot);

  // Enrollment (two-step)
  static FPResult enrollStep1(uint8_t slotID);
  static FPResult enrollStep2(uint8_t slotID);

  // Management
  static bool     deleteSlot(uint8_t slotID);
  static bool     deleteAll();
  static uint8_t  getTemplateCount();
  static FPSlotMap loadSlotMap();
  static bool     saveSlotMap(const FPSlotMap& map);

  // LED (R503 RGB)
  static void     ledBlue();
  static void     ledGreen();
  static void     ledRed();
  static void     ledOff();

private:
  static Adafruit_Fingerprint _sensor;
  static bool                 _ready;
  static bool                 _verifyHWPassword();
};
