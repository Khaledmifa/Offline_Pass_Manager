#pragma once
#include <Arduino.h>
// =============================================================
//  Battery.h | LiPo voltage via 100K/100K ADC divider
// =============================================================
class Battery {
public:
  static void    begin();
  static uint8_t getPercent();      // 0-100
  static uint16_t getVoltage_mV();  // raw mV
  static bool    isCritical();      // < BAT_CRITICAL_MV
  static bool    isCharging();      // optional: TP4056 CHRG pin
};
