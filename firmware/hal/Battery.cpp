#include "Battery.h"
#include "../config.h"

void Battery::begin() {
  pinMode(BAT_ADC_PIN, INPUT);
  analogReadResolution(12);
}

uint16_t Battery::getVoltage_mV() {
  uint32_t sum = 0;
  for (int i = 0; i < BAT_ADC_SAMPLES; i++) {
    sum += analogRead(BAT_ADC_PIN);
    delayMicroseconds(200);
  }
  uint32_t raw = sum / BAT_ADC_SAMPLES;
  // ADC raw -> mV at pin -> actual battery mV (×divider ratio)
  return (uint16_t)((raw * BAT_VREF_MV / JOY_ADC_MAX) * BAT_DIVIDER);
}

uint8_t Battery::getPercent() {
  uint16_t mv = getVoltage_mV();
  if (mv >= BAT_FULL_MV)  return 100;
  if (mv <= BAT_EMPTY_MV) return 0;
  return (uint8_t)((mv - BAT_EMPTY_MV) * 100UL / (BAT_FULL_MV - BAT_EMPTY_MV));
}

bool Battery::isCritical() {
  return getVoltage_mV() < BAT_CRITICAL_MV;
}

bool Battery::isCharging() {
  // Tie TP4056 CHRG pin to a GPIO if wired; returns false otherwise
  return false;
}
