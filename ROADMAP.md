# Hardware Password Manager — Development Roadmap
**Total Estimated Time:** 8–10 weeks (solo developer)

---

## Phase 0 — Environment Setup (Day 1–2)
**Goal:** Working build environment that compiles and flashes to the board.

- [ ] Install Arduino IDE 2.x + ESP32 Core ≥ 2.0.14
- [ ] Configure board settings (ESP32-S3, 16MB, OPI PSRAM, USB-OTG)
- [ ] Create `partitions.csv` for 13MB LittleFS
- [ ] Flash empty sketch; confirm USB serial output
- [ ] Install all libraries (U8g2, Adafruit FP, NimBLE, ArduinoJson, etc.)
- [ ] Wire OLED → ESP32-S3 (SDA=8, SCL=9); confirm I2C scan
- [ ] Upload U8g2 Hello World; confirm landscape rotation (U8G2_R1)
- [ ] Wire Joystick; print ADC values via Serial; confirm axes + button

---

## Phase 1 — HAL Layer (Week 1)
**Goal:** All hardware drivers working and tested independently.

### 1.1 Display (hal/Display.h)
- [ ] SH1107 init with U8G2_R1 rotation → 128×64 landscape confirmed
- [ ] Status bar: battery icon, HID badge, device name
- [ ] Implement `drawBootSplash()`
- [ ] Implement `drawMainMenu()` with selection highlight + scrollbar
- [ ] Implement `drawPINEntry()` with digit boxes
- [ ] Implement all remaining screen functions
- [ ] Font selection: FONT_TINY / FONT_SMALL / FONT_NORMAL / FONT_BOLD
- [ ] Test Arabic font (u8g2_font_unifont_t_arabic) and CJK font

### 1.2 Joystick (hal/Joystick.h)
- [ ] ADC read with 10% deadzone
- [ ] Direction event detection (UP/DOWN/LEFT/RIGHT)
- [ ] Hold-repeat every 150ms
- [ ] Button debounce 20ms
- [ ] Test all 5 event types via Serial

### 1.3 Battery (hal/Battery.h)
- [ ] Wire voltage divider (100K/100K) to GPIO4
- [ ] 16-sample ADC averaging
- [ ] Voltage → percent mapping (LiPo curve)
- [ ] `isCritical()` threshold test

### 1.4 Fingerprint (hal/Fingerprint.h)
- [ ] Wire R503: VCC=3.3V, GND, TX→GPIO18, RX→GPIO17, WAKEUP→GPIO5
- [ ] UART2 init at 57600 baud
- [ ] `begin()` with `verifyPassword(FP_HW_PASSWORD)`
- [ ] `enrollStep1()` and `enrollStep2()` — two-scan enrollment
- [ ] `verify()` — search and match
- [ ] `deleteSlot()` and `deleteAll()`
- [ ] RGB LED control (blue/green/red/off)
- [ ] Test deep-sleep wakeup via GPIO5 EXT0

---

## Phase 2 — Core Services (Week 2)
**Goal:** Encryption, storage, and multi-language working.

### 2.1 Security (core/Security.h)
- [ ] `generateSalt()` using `esp_random()`
- [ ] `saveSalt()` / `loadSalt()` via Storage
- [ ] `deriveKey()` — PBKDF2-SHA256 with 100,000 rounds
- [ ] `zeroKey()` — memset master key to zero
- [ ] `encrypt()` — AES-256-CBC with random IV, returns base64(IV||cipher)
- [ ] `decrypt()` — extract IV, decrypt, remove PKCS#7 padding
- [ ] `hashPIN()` — PBKDF2 for PIN verification
- [ ] `verifyPIN()` — constant-time compare
- [ ] Unit test: encrypt then decrypt known string → identical

### 2.2 Storage (core/Storage.h)
- [ ] LittleFS `begin()` with format-on-fail
- [ ] `isFirstBoot()` — check for config.json
- [ ] `_atomicWrite()` — write to .tmp then rename
- [ ] `saveSalt()` / `loadSalt()`
- [ ] `saveConfig()` / `loadConfig()` — all DeviceConfig fields
- [ ] `saveProfile()` / `loadProfile()` — user registration data
- [ ] `saveNetwork()` / `loadNetwork()` — encrypted WiFi + SMTP
- [ ] `saveVault()` / `loadVault()` — credential array
- [ ] `appendCredential()` — append single credential
- [ ] `readVaultBlob()` — raw bytes for email attachment
- [ ] Test: write + read + corrupt + recover

### 2.3 Language (core/Language.h)
- [ ] Define `StringKey` enum (all UI strings)
- [ ] PROGMEM string tables for EN, AR, DE, FR, TR, ZH
- [ ] `getString(key, lang)` returning `const char*`
- [ ] `isRTL(lang)` for Arabic right-to-left flag
- [ ] Integrate with Display.h — all hardcoded strings replaced

---

## Phase 3 — UI State Machine (Week 3)
**Goal:** Full menu navigation working with joystick.

### 3.1 Core FSM (app/UI.h)
- [ ] Define `AppState` enum (all 28 states)
- [ ] `setState()` with `onEnter()` hook
- [ ] `loop()` → `pollJoystick()` → `handleInput()` → `render()`
- [ ] 30-second inactivity watchdog (`millis()` based)
- [ ] Back-stack for ◄ navigation
- [ ] Battery critical overlay trigger

### 3.2 Locked Screen
- [ ] `LOCKED_IDLE` → draw lock screen
- [ ] Touch wakeup → `UNLOCK_FINGERPRINT`
- [ ] Button press → `UNLOCK_PIN`

### 3.3 PIN Entry
- [ ] `UNLOCK_PIN` → digit entry with joystick
- [ ] Wrong attempt counter (max 5)
- [ ] Penalty delay after max attempts

### 3.4 Main Menu
- [ ] 5 items with scroll + highlight
- [ ] Navigate to each sub-section

### 3.5 Credentials UI
- [ ] Scrollable credential list
- [ ] Credential detail view
- [ ] Send mode selector (U+P / U / P)

### 3.6 Password Generator UI
- [ ] Options screen (length + charset flags)
- [ ] Generated result with horizontal scroll
- [ ] Save to new credential

### 3.7 Settings UI
- [ ] Language switcher
- [ ] HID mode toggle (USB/BLE)
- [ ] Auto-lock duration selector
- [ ] Fingerprint management
- [ ] Manual backup trigger
- [ ] About screen

---

## Phase 4 — Authentication (Week 4)
**Goal:** Full unlock flow working end-to-end.

- [ ] Fingerprint unlock → key derive → load vault
- [ ] PIN unlock → `verifyPIN()` → key derive → load vault
- [ ] Decrypt all credentials on unlock (store plaintext in RAM vector)
- [ ] `zeroKey()` + clear RAM vector on lock
- [ ] Enroll new fingerprint from Settings
- [ ] Delete fingerprint from Settings
- [ ] Test: lock → sleep → touch wakeup → fingerprint → main menu

---

## Phase 5 — HID Output (Week 5)
**Goal:** Credentials type correctly on USB and BLE.

### 5.1 USB HID (comm/HID_Manager.h)
- [ ] `USB.begin()` + `USBHIDKeyboard keyboard`
- [ ] `typeString()` with 10ms inter-key delay
- [ ] Handle special characters (!@#$ etc.)
- [ ] Send Tab between username and password
- [ ] Test on Windows + macOS + Linux

### 5.2 BLE HID
- [ ] NimBLE-Arduino HID device setup
- [ ] HID keyboard descriptor
- [ ] `typeString()` via BLE
- [ ] BLE pairing + reconnect logic
- [ ] Settings toggle: USB ↔ BLE (save to config.json)

---

## Phase 6 — First Boot Web Setup (Week 6)
**Goal:** AP captive portal + registration + restore working.

- [ ] Upload `index.html` to LittleFS `/web/`
- [ ] `WiFiManager::startSetupAP()` — WPA2 AP + DNS server
- [ ] DNS redirect all queries to 192.168.4.1
- [ ] `ESPAsyncWebServer` serves `/web/index.html`
- [ ] `POST /api/register` handler → validate + save profile
- [ ] `POST /api/restore` handler → verify password + restore vault
- [ ] `GET /api/status` → JSON device state
- [ ] AP auto-closes after registration success
- [ ] Transition to `WIZARD_LANGUAGE`

---

## Phase 7 — On-Device Wizard (Week 6–7)

- [ ] Language selection + save to config
- [ ] PIN set + confirm + hash + save
- [ ] Fingerprint enrollment loop (1–5 fps)
- [ ] "Add another?" prompt after each
- [ ] Backup frequency selection + save
- [ ] Transition to `WIFI_EMAIL_AP`

---

## Phase 8 — WiFi & Email Setup (Week 7)

- [ ] Upload `wifi_setup.html` to LittleFS `/web/`
- [ ] Start second AP after wizard complete
- [ ] `POST /api/test-email` → SMTP test connection
- [ ] `POST /api/save-network` → encrypt + save to network.json
- [ ] Connect to home WiFi after save
- [ ] Display "Setup Complete" + transition to `LOCKED_IDLE`

---

## Phase 9 — Email Backup (Week 8)

- [ ] `EmailManager::configure()` from network.json
- [ ] `sendBackup()` → attach encrypted vault.json blob
- [ ] Send to `to_user` AND `to_admin` in one SMTP session
- [ ] Subject: `"PM Backup — YYYY-MM-DD — [deviceID]"`
- [ ] Manual backup from Settings menu
- [ ] Scheduled backup: millis() / RTC check against `backup_freq_days`
- [ ] Retry on fail (max 3 attempts)

---

## Phase 10 — Polish & Testing (Week 9–10)

### Bug Fix & Stability
- [ ] Test all 28 UI states exhaustively
- [ ] Test power-loss recovery (corrupt vault.json scenario)
- [ ] Test wrong PIN lockout + penalty
- [ ] Test R503 sensor removal detection
- [ ] Test BLE reconnect after phone disconnects
- [ ] Measure battery life (target >4 hours)

### UI Polish
- [ ] Smooth scrollbar animation
- [ ] Consistent font usage across all screens
- [ ] Arabic RTL alignment in all text fields
- [ ] OLED dim-before-off transition

### Performance
- [ ] Boot to locked screen < 3 seconds
- [ ] Fingerprint verify < 2 seconds
- [ ] Vault load (200 credentials) < 500ms

### Security Audit
- [ ] Verify key is zeroed on every lock path
- [ ] Verify no plaintext in vault.json (spot check)
- [ ] Verify AP password not exposed in any API response
- [ ] Confirm PBKDF2 rounds meet target (100,000)

### Documentation
- [ ] Pin wiring diagram (Fritzing or SVG)
- [ ] User manual (simple PDF)
- [ ] Flash + setup guide

---

## Milestone Summary

| Milestone | Target Week | Deliverable |
|---|---|---|
| M0: Environment | Week 1 | Compiles, flashes, OLED shows text |
| M1: HAL Complete | Week 1 | All hardware drivers tested |
| M2: Core Complete | Week 2 | Encrypt/decrypt + storage working |
| M3: Full UI | Week 3 | All menus navigate correctly |
| M4: Auth Working | Week 4 | Fingerprint + PIN unlock vault |
| M5: HID Output | Week 5 | Types credentials on PC/phone |
| M6: Web Setup | Week 6 | Captive portal + registration |
| M7: Full Setup Flow | Week 7 | Complete first-boot to locked |
| M8: Email Backup | Week 8 | Encrypted vault emailed on schedule |
| M9: Production Ready | Week 10 | All edge cases handled, tested |
