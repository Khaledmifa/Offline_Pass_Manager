#include "HID_Manager.h"
#include "../config.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

static USBHIDKeyboard usbKbd;

// BLE HID globals
static NimBLEHIDDevice* bleHID  = nullptr;
static NimBLECharacteristic* bleInput = nullptr;
static bool bleConnected = false;

class BLECallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* s)    override { bleConnected = true;  }
  void onDisconnect(NimBLEServer* s) override {
    bleConnected = false;
    NimBLEDevice::startAdvertising();
  }
};

HIDMode HID_Manager::_mode = HIDMode::USB;

void HID_Manager::begin(HIDMode mode) {
  _mode = mode;
  if (mode == HIDMode::USB) {
    USB.begin();
    usbKbd.begin();
  } else {
    NimBLEDevice::init("PwdMgr");
    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(new BLECallbacks());
    bleHID = new NimBLEHIDDevice(server);
    bleInput = bleHID->inputReport(1);
    bleHID->manufacturer()->setValue("PM-Device");
    bleHID->pnp(0x02, 0x045e, 0x07a5, 0x0111);
    bleHID->hidInfo(0x00, 0x01);
    bleHID->startServices();
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->setAppearance(HID_KEYBOARD);
    adv->addServiceUUID(bleHID->hidService()->getUUID());
    adv->start();
  }
}

void HID_Manager::setMode(HIDMode m) { _mode = m; }
HIDMode HID_Manager::getMode()       { return _mode; }

bool HID_Manager::isConnected() {
  if (_mode == HIDMode::USB) return true;
  return bleConnected;
}

void HID_Manager::_typeChar(char c) {
  if (_mode == HIDMode::USB) {
    usbKbd.print(c);
  }
  // BLE: send HID report (simplified — full keymap needed for production)
  delay(HID_KEY_DELAY_MS);
}

void HID_Manager::typeString(const char* s) {
  if (_mode == HIDMode::USB) {
    usbKbd.print(s);
  } else {
    for (const char* p = s; *p; p++) _typeChar(*p);
  }
}

void HID_Manager::pressTab()   {
  if (_mode == HIDMode::USB) usbKbd.write(KEY_TAB);
}
void HID_Manager::pressEnter() {
  if (_mode == HIDMode::USB) usbKbd.write(KEY_RETURN);
}

void HID_Manager::typeUsername(const char* u) { typeString(u); }
void HID_Manager::typePassword(const char* p) { typeString(p); }

void HID_Manager::typeBoth(const char* u, const char* p) {
  typeString(u);
  pressTab();
  delay(HID_FIELD_DELAY_MS);
  typeString(p);
}
