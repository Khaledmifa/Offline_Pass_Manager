# Hardware Password Manager — ESP32-S3
**Version:** 1.0.0 | **Platform:** ESP32-S3 (N16R8 — 16MB Flash)

---

## What is this?

A standalone offline hardware device that stores, encrypts, and auto-types
your passwords via USB or Bluetooth keyboard emulation.
No cloud service is required for core functionality.
All credentials are encrypted with AES-256 and stored locally.

---

## Hardware

| Component | Model | Connection |
|---|---|---|
| MCU | ESP32-S3-DevKitC-1 N16R8 | — |
| Display | 1.3" SH1107 OLED 64×128 | I2C (GPIO 8/9) |
| Fingerprint | GROW R503 (15mm) | UART2 (GPIO 17/18) |
| Joystick | XY + SW button | ADC (GPIO 1/2) + GPIO 3 |
| Battery | 3.7V LiPo 800mAh + TP4056 | ADC divider (GPIO 4) |

---

## Quick Setup (3 Phases)

### Phase 1 — Web Registration
1. Power on device → broadcasts **PwdMgr_Setup** (WPA2)
2. Connect phone to WiFi: **PwdMgr_Setup**
3. Password: `Pm@9Secure!26x` ← fixed, cannot change
4. Open browser → `http://192.168.4.1`
5. Fill in: email, password, name, DOB, phone, company (opt.), **admin email**
6. Tap **Create Account** → page shows "Continue on device"

### Phase 2 — On-Device Wizard
1. Select language with joystick (↑↓ scroll, ► select)
2. Set device PIN — **minimum 8 characters** including:
   - At least 1 uppercase letter (A–Z)
   - At least 1 lowercase letter (a–z)
   - At least 1 digit (0–9)
   - At least 1 symbol (!@#$% etc.)
   - English characters only
   - Entered via joystick character cycling
3. Enroll fingerprints (1–5, two scans each)
4. Device shows: **"Connect to PwdMgr_Setup to finish"**

### Phase 3 — WiFi & Email Setup
1. Reconnect phone to **PwdMgr_Setup** (same password)
2. Open browser → `http://192.168.4.1`
3. Enter home WiFi credentials
4. Enter SMTP account for sending backups
5. Confirm personal backup inbox + admin email (pre-filled)
6. Tap **Save & Complete Setup** → device connects to home WiFi → done

---

## AP Password Rules

The setup network password is **hardcoded in firmware**:
- Current value: `Pm@9Secure!26x`
- Length: 14 characters
- Contains: uppercase, lowercase, digit, symbol
- **Cannot be changed by user or admin**
- Same for every device of this firmware build
- To change: edit `SETUP_AP_PASS` in `config.h` and recompile

---

## Device PIN Rules

| Rule | Requirement |
|---|---|
| Minimum length | 8 characters |
| Uppercase (A-Z) | At least 1 |
| Lowercase (a-z) | At least 1 |
| Digits (0-9) | At least 1 |
| Symbols (!@#$) | At least 1 |
| Language | English / ASCII only |
| Entry method | Joystick (↑↓ cycle, ► confirm, ◄ backspace) |
| Lockout | 5 wrong attempts → 30s penalty |

---

## File Structure

```
firmware/
├── firmware.ino          Main sketch
├── config.h              All pins, constants, AP password
├── partitions.csv        16MB flash layout
│
├── hal/                  Hardware Abstraction Layer
│   ├── Display.*         SH1107 OLED (128×64 landscape)
│   ├── Joystick.*        ADC axes + button
│   ├── Fingerprint.*     GROW R503 UART2 driver
│   └── Battery.*         Voltage divider reader
│
├── core/                 Core Services
│   ├── Security.*        AES-256-CBC + PBKDF2 + PIN validation
│   ├── Storage.*         LittleFS vault + config
│   ├── Language.*        6-language string tables
│   └── SiteDatabase.*    ~260 site names for autocomplete
│
├── app/                  Application Logic
│   ├── UI.*              State machine (28 states)
│   ├── UI_PIN.*          Alphanumeric PIN wizard
│   ├── UI_additions.*    Admin setup + add credential flow
│   ├── CharInput.*       Joystick text entry + autocomplete
│   ├── PasswordManager.* Credential list + HID dispatch
│   └── PasswordGenerator.* Hardware TRNG password generation
│
├── comm/                 Communication
│   ├── HID_Manager.*     USB HID + BLE HID unified output
│   ├── WiFiManager.*     AP setup + captive portal + STA
│   └── EmailManager.*    SMTP encrypted backup sender
│
└── web/                  Stored in LittleFS /web/
    ├── index.html        Registration + Restore portal
    └── wifi_setup.html   WiFi + Email configuration
```

---

## Arduino IDE Build Settings

| Setting | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| Flash Size | 16MB (128Mb) |
| Partition Scheme | Custom (use `partitions.csv`) |
| PSRAM | OPI PSRAM |
| USB Mode | USB-OTG (TinyUSB) |
| USB CDC On Boot | Enabled |

## Required Libraries (Library Manager)

```
U8g2                        by olikraus
Adafruit Fingerprint Sensor by Adafruit
NimBLE-Arduino              by h2zero
ArduinoJson                 by Benoit Blanchon  (v7)
ESP32-Mail-Client           by mobizt
```

## Manual Install (GitHub)
```bash
git clone https://github.com/me-no-dev/ESPAsyncWebServer
git clone https://github.com/me-no-dev/AsyncTCP
# Copy both into ~/Arduino/libraries/
```

---

## Security Architecture

```
Registration password  ──►  PBKDF2-SHA256  ──►  stored hash
Device PIN            ──►  PBKDF2-SHA256  ──►  AES-256 master key
                                                (RAM only, zeroed on lock)

Each credential:
  plaintext  ──►  AES-256-CBC  ──►  base64(random_IV || ciphertext)
                 (random IV per save)

Backup emails:
  vault.json blob (already encrypted)  ──►  SMTP attachment
  User copy  +  Admin copy sent simultaneously
  Admin CANNOT decrypt without user's PIN
```

---

## Documents in this Package

| File | Contents |
|---|---|
| `README.md` | This file |
| `PRD.md` | Product requirements (60+ requirements) |
| `TECHNICAL_SPEC.md` | Full architecture, data models, API, edge cases |
| `ROADMAP.md` | 10-phase development plan, 80+ tasks |
| `SETUP_FLOW.md` | Corrected 3-phase setup flow (final version) |
| `CORRECTIONS.md` | What changed from initial design |
