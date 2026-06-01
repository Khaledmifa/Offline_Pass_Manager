#include "Fingerprint.h"
#include "../config.h"
#include "../core/Storage.h"
#include <HardwareSerial.h>

HardwareSerial fpSerial(2);   // UART2
Adafruit_Fingerprint Fingerprint::_sensor(&fpSerial);
bool Fingerprint::_ready = false;

bool Fingerprint::begin() {
  fpSerial.begin(FP_BAUD, SERIAL_8N1, FP_RX_PIN, FP_TX_PIN);
  delay(100);
  if (!_sensor.begin(FP_BAUD)) { Serial.println("[FP] Sensor not found"); return false; }
  if (!_verifyHWPassword())     { Serial.println("[FP] Wrong HW password — tampered?"); return false; }
  _ready = true;
  ledOff();
  return true;
}

bool Fingerprint::isReady() { return _ready; }

bool Fingerprint::_verifyHWPassword() {
  _sensor.setPassword(FP_HW_PASSWORD);
  return (_sensor.verifyPassword() == FINGERPRINT_OK);
}

FPResult Fingerprint::verify(uint8_t& matchedSlot) {
  ledBlue();
  uint32_t start = millis();
  // Wait for finger
  while (_sensor.getImage() != FINGERPRINT_OK) {
    if (millis() - start > FP_VERIFY_TIMEOUT) { ledOff(); return FPResult::TIMEOUT; }
    delay(50);
  }
  if (_sensor.image2Tz()      != FINGERPRINT_OK) { ledRed(); return FPResult::FAIL; }
  if (_sensor.fingerFastSearch() != FINGERPRINT_OK) {
    ledRed(); delay(800); ledOff();
    return FPResult::FAIL;
  }
  matchedSlot = _sensor.fingerID;
  ledGreen(); delay(600); ledOff();
  return FPResult::OK;
}

FPResult Fingerprint::enrollStep1(uint8_t slotID) {
  ledBlue();
  uint32_t start = millis();
  while (_sensor.getImage() != FINGERPRINT_OK) {
    if (millis() - start > FP_ENROLL_TIMEOUT) { ledOff(); return FPResult::TIMEOUT; }
    delay(60);
  }
  if (_sensor.image2Tz(1) != FINGERPRINT_OK) { ledRed(); return FPResult::FAIL; }
  ledGreen(); delay(300); ledOff();
  return FPResult::ENROLL_STEP1_DONE;
}

FPResult Fingerprint::enrollStep2(uint8_t slotID) {
  // Wait for finger lift
  while (_sensor.getImage() == FINGERPRINT_OK) delay(50);
  delay(200);
  // Second scan
  ledBlue();
  uint32_t start = millis();
  while (_sensor.getImage() != FINGERPRINT_OK) {
    if (millis() - start > FP_ENROLL_TIMEOUT) { ledOff(); return FPResult::TIMEOUT; }
    delay(60);
  }
  if (_sensor.image2Tz(2)     != FINGERPRINT_OK) { ledRed(); return FPResult::FAIL; }
  if (_sensor.createModel()   != FINGERPRINT_OK) { ledRed(); return FPResult::FAIL; }
  if (_sensor.storeModel(slotID) != FINGERPRINT_OK) { ledRed(); return FPResult::FAIL; }
  ledGreen(); delay(600); ledOff();
  return FPResult::OK;
}

bool Fingerprint::deleteSlot(uint8_t slotID) {
  return (_sensor.deleteModel(slotID) == FINGERPRINT_OK);
}

bool Fingerprint::deleteAll() {
  return (_sensor.emptyDatabase() == FINGERPRINT_OK);
}

uint8_t Fingerprint::getTemplateCount() {
  _sensor.getTemplateCount();
  return _sensor.templateCount;
}

FPSlotMap Fingerprint::loadSlotMap() {
  FPSlotMap m;
  // Read from config.json via Storage
  // (Storage::loadFPSlotMap implemented in Storage.cpp)
  return m;
}

bool Fingerprint::saveSlotMap(const FPSlotMap& map) {
  return true; // delegated to Storage
}

void Fingerprint::ledBlue()  { _sensor.LEDcontrol(FINGERPRINT_LED_BREATHING, 50, FINGERPRINT_LED_BLUE);   }
void Fingerprint::ledGreen() { _sensor.LEDcontrol(FINGERPRINT_LED_FLASHING,  25, FINGERPRINT_LED_GREEN);  }
void Fingerprint::ledRed()   { _sensor.LEDcontrol(FINGERPRINT_LED_FLASHING,  25, FINGERPRINT_LED_RED);    }
void Fingerprint::ledOff()   { _sensor.LEDcontrol(FINGERPRINT_LED_GRADUAL_OFF, 0, FINGERPRINT_LED_BLUE);  }
