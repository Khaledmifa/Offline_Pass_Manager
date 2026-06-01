// =============================================================
//  Hardware Password Manager — firmware.ino
//  ESP32-S3 (N16R8) | Arduino Core 2.x | v1.0.0
//
//  Board settings (Arduino IDE):
//    Board:            ESP32S3 Dev Module
//    Flash Size:       16MB (128Mb)
//    Partition Scheme: Custom  (use partitions.csv)
//    PSRAM:            OPI PSRAM
//    USB Mode:         USB-OTG (TinyUSB)
//    USB CDC On Boot:  Enabled
// =============================================================

// ── HAL ──────────────────────────────────────────────────────
#include "hal/Display.h"
#include "hal/Joystick.h"
#include "hal/Battery.h"
#include "hal/Fingerprint.h"

// ── Core ─────────────────────────────────────────────────────
#include "core/Security.h"
#include "core/Storage.h"
#include "core/Language.h"

// ── App ──────────────────────────────────────────────────────
#include "app/UI.h"
#include "app/PasswordManager.h"
#include "app/PasswordGenerator.h"

// ── Comm ─────────────────────────────────────────────────────
#include "comm/HID_Manager.h"
#include "comm/WiFiManager.h"
#include "comm/EmailManager.h"

// ── Web server (used during AP phases) ───────────────────────
#include <ESPAsyncWebServer.h>
static AsyncWebServer webServer(80);
static bool           serverStarted = false;

// ── Backup scheduler ─────────────────────────────────────────
static uint32_t lastBackupMs  = 0;
static bool     backupEnabled = false;

// =============================================================
//  DEEP SLEEP WAKEUP
// =============================================================
void IRAM_ATTR onTouchWakeup() {
  // EXT0 wakeup ISR — no action needed; device wakes automatically
}

void setupDeepSleepWakeup() {
  esp_sleep_enable_ext0_wakeup(FP_WAKEUP_PIN, HIGH);
}

// =============================================================
//  SETUP
// =============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n[PM] Hardware Password Manager v" FW_VERSION " booting...");

  // ── Check wakeup cause ────────────────────────────────────
  esp_sleep_wakeup_cause_t wakeup = esp_sleep_get_wakeup_cause();
  bool fromSleep = (wakeup == ESP_SLEEP_WAKEUP_EXT0);
  if (fromSleep) Serial.println("[PM] Woke from deep sleep (touch)");

  // ── Hardware init ─────────────────────────────────────────
  Battery::begin();
  Joystick::begin();
  Display::begin();
  Display::drawBootSplash(FW_VERSION);

  // ── File system ───────────────────────────────────────────
  if (!Storage::begin()) {
    Display::drawMessage("Fatal Error", "LittleFS failed", true);
    while (true) delay(1000);
  }

  // ── Load config & salt ────────────────────────────────────
  DeviceConfig cfg;
  bool firstBoot = Storage::isFirstBoot();
  if (!firstBoot) {
    Storage::loadConfig(cfg);
    Security::loadSalt();
  }

  // ── Fingerprint sensor ────────────────────────────────────
  if (!Fingerprint::begin()) {
    Serial.println("[FP] Sensor not found — PIN-only mode");
    // Device continues in PIN-only mode; fingerprint states will be skipped
  }

  // ── HID ───────────────────────────────────────────────────
  HID_Manager::begin(cfg.hid_ble ? HIDMode::BLE : HIDMode::USB);

  // ── Language ──────────────────────────────────────────────
  Language::setLanguage((Lang)cfg.language);

  // ── WiFi manager ──────────────────────────────────────────
  WiFiManager::begin();

  // ── UI FSM ────────────────────────────────────────────────
  UI::begin();

  // On first boot, start AP immediately after splash
  if (firstBoot) {
    delay(2000);
    WiFiManager::startAP(APPhase::REGISTRATION);
    WiFiManager::setupRoutes(webServer, APPhase::REGISTRATION);
    webServer.begin();
    serverStarted = true;
    UI::setState(AppState::FIRST_BOOT_AP);
  } else if (fromSleep) {
    UI::setState(AppState::UNLOCK_FINGERPRINT);
  } else {
    UI::setState(AppState::LOCKED_IDLE);
  }

  // ── Deep sleep wakeup config ──────────────────────────────
  setupDeepSleepWakeup();

  // ── Backup scheduler ──────────────────────────────────────
  if (cfg.wifiSetupDone) {
    backupEnabled = true;
    lastBackupMs  = millis();
  }

  Serial.println("[PM] Setup complete");
}

// =============================================================
//  LOOP
// =============================================================
void loop() {
  // ── Core FSM tick ─────────────────────────────────────────
  UI::tick();

  // ── Start web server for WiFi-email AP phase ──────────────
  AppState state = UI::getState();
  if (state == AppState::WIFI_EMAIL_AP && !serverStarted) {
    WiFiManager::setupRoutes(webServer, APPhase::NETWORK_SETUP);
    webServer.begin();
    serverStarted = true;
  }

  // ── Tear down web server after AP phases ──────────────────
  if (serverStarted &&
      state != AppState::FIRST_BOOT_AP &&
      state != AppState::WIFI_EMAIL_AP &&
      !WiFiManager::isAPActive()) {
    // AsyncWebServer has no stop(); just flag it off
    serverStarted = false;
  }

  // ── Scheduled backup ──────────────────────────────────────
  if (backupEnabled && WiFiManager::isConnected()) {
    DeviceConfig cfg;
    Storage::loadConfig(cfg);
    uint32_t intervalMs = (uint32_t)cfg.backup_freq_days * 86400000UL;
    if (millis() - lastBackupMs > intervalMs) {
      lastBackupMs = millis();
      Serial.println("[PM] Running scheduled backup...");
      bool ok = EmailManager::sendBackup();
      Serial.printf("[PM] Backup %s\n", ok ? "sent" : "failed");
    }
  }

  // ── Enter deep sleep when device is locked ─────────────────
  static uint32_t lockedSince = 0;
  if (state == AppState::LOCKED_IDLE) {
    if (lockedSince == 0) lockedSince = millis();
    if (millis() - lockedSince > OLED_OFF_DELAY_MS) {
      Serial.println("[PM] Entering deep sleep...");
      Display::sleep();
      esp_deep_sleep_start();
    }
  } else {
    lockedSince = 0;
  }

  delay(10);   // ~100 Hz tick rate
}
