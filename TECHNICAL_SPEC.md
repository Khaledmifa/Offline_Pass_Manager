# Hardware Password Manager — Technical Specification
**Version:** 1.0.0 | **Platform:** ESP32-S3 (N16R8) | **Framework:** Arduino / ESP-IDF

---

## 1. Hardware Bill of Materials

| Component | Model | Interface | Notes |
|---|---|---|---|
| MCU | ESP32-S3-DevKitC-1 (N16R8) | — | 16MB Flash, 8MB PSRAM, native USB |
| Display | 1.3" SH1107 OLED 64×128 | I2C | Mounted portrait, rotated 90° in SW |
| Fingerprint | GROW R503 (15mm) | UART2 | RGB LED, 200 templates, HW password |
| Joystick | XY Dual-Axis + SW button | ADC + GPIO | 10% deadzone in SW |
| Charging | TP4056 1A 3.7V board | — | Charges 800mAh LiPo |
| Battery | 3.7V LiPo 800mAh | ADC divider | 100K/100K → ADC pin |

---

## 2. Pin Assignment

| Signal | ESP32-S3 GPIO | Notes |
|---|---|---|
| OLED SDA | GPIO 8 | I2C default |
| OLED SCL | GPIO 9 | I2C default |
| FP UART2 RX | GPIO 18 | From R503 TX |
| FP UART2 TX | GPIO 17 | To R503 RX |
| FP WAKEUP | GPIO 5 | EXT0 deep-sleep wakeup |
| Joystick X (VRx) | GPIO 1 / ADC1_CH0 | Analog |
| Joystick Y (VRy) | GPIO 2 / ADC1_CH1 | Analog |
| Joystick SW | GPIO 3 | INPUT_PULLUP |
| Battery ADC | GPIO 4 / ADC1_CH3 | After 100K/100K divider |
| USB D- | GPIO 19 | Native USB (built-in) |
| USB D+ | GPIO 20 | Native USB (built-in) |

---

## 3. Software Architecture

### 3.1 Layer Diagram

```
┌────────────────────────────────────────────────────────┐
│                   firmware.ino                         │
│           setup() ──── loop() ──── ISR                 │
└────────────────┬───────────────────────────────────────┘
                 │
┌────────────────▼───────────────────────────────────────┐
│              Application Layer  (app/)                 │
│   UI.h (FSM)  │  PasswordManager.h  │  PwdGenerator.h │
└────┬─────────────────────────┬──────────────────┬──────┘
     │                         │                  │
┌────▼──────────┐   ┌──────────▼──────┐   ┌───────▼──────┐
│  Core Layer   │   │  Comm Layer     │   │  HAL Layer   │
│  (core/)      │   │  (comm/)        │   │  (hal/)      │
│               │   │                 │   │              │
│ Security.h    │   │ HID_Manager.h   │   │ Display.h    │
│ Storage.h     │   │ WiFiManager.h   │   │ Joystick.h   │
│ Language.h    │   │ EmailManager.h  │   │ Fingerprint.h│
└───────────────┘   └─────────────────┘   │ Battery.h    │
                                           └──────────────┘
```

### 3.2 Technology Stack

| Layer | Technology | Version |
|---|---|---|
| Framework | Arduino + ESP-IDF | Core 2.x |
| Display driver | U8g2 by olikraus | Latest |
| Fingerprint | Adafruit Fingerprint Sensor Library | Latest |
| BLE HID | NimBLE-Arduino by h2zero | Latest |
| USB HID | USBHIDKeyboard (built-in ESP32 Core) | — |
| Encryption | mbedTLS AES + PKCS5 (built-in ESP-IDF) | — |
| File System | LittleFS (built-in ESP32 Core) | — |
| JSON | ArduinoJson by Benoit Blanchon | v7 |
| Web Server | ESPAsyncWebServer | Latest |
| Async TCP | AsyncTCP (ESP32) | Latest |
| Email | ESP32-Mail-Client by mobizt | Latest |
| HTTPS | WiFiClientSecure (built-in) | — |

---

## 4. Security Architecture

### 4.1 Key Derivation

```
User PIN  ──►  PBKDF2-SHA256  ──►  AES-256 Master Key
               │                   │
               │ 100,000 rounds     │ Lives in RAM only
               │ 16-byte salt       │ Zeroed on lock
               │ (stored in flash)  │
               └────────────────────┘
```

### 4.2 Credential Encryption

```
Plaintext credential
        │
        ▼
  Random 16-byte IV  ──────────────────────┐
        │                                   │
        ▼                                   │
  AES-256-CBC encrypt with master key      │
        │                                   │
        ▼                                   ▼
  Ciphertext  ──► concat(IV, ciphertext) ──► base64 ──► stored in vault.json
```

### 4.3 Admin Authentication Model

```
Admin wants to reconfigure device
        │
        ▼
  Connects to "PwdMgr_Setup" AP
  [requires WPA2 password = SETUP_AP_PASS]
        │
        ▼
  Authenticated ── connection to AP IS the credential
  No separate admin password stored anywhere
        │
        ▼
  Access to setup portal / device reconfiguration
```

### 4.4 R503 Anti-Spoofing

```
ESP32 ──UART──► R503: verifyPassword(0xA5B6C7D8)
                R503: ──► FINGERPRINT_OK or FAIL
                          │
                          ▼ FAIL
               emergencyLock() ── sensor tampered/replaced
```

### 4.5 Backup Security

```
vault.json (AES-256 encrypted blob)
        │
        ├──► User email  (can decrypt with own PIN)
        │
        └──► Admin email (holds file; CANNOT decrypt without user PIN)
```

---

## 5. Data Models

### 5.1 vault.json
```json
{
  "credentials": [
    {
      "name":         "GitHub",
      "username_enc": "base64(IV||AES256(username))",
      "password_enc": "base64(IV||AES256(password))"
    }
  ]
}
```

### 5.2 config.json
```json
{
  "salt":             "base64(16-byte PBKDF2 salt)",
  "pin_hash":         "base64(PBKDF2(PIN, salt, 10000rounds))",
  "language":         0,
  "autolock_ms":      30000,
  "backup_freq_days": 7,
  "hid_ble":          false,
  "fp_slot_count":    2,
  "fp_slots":         [true, true, false, false, false],
  "setup_done":       true,
  "wifi_setup_done":  true
}
```

### 5.3 network.json
```json
{
  "wifi_ssid":    "base64(IV||AES256(ssid))",
  "wifi_pass":    "base64(IV||AES256(pass))",
  "smtp_server":  "smtp.gmail.com",
  "smtp_port":    587,
  "smtp_email":   "user@gmail.com",
  "smtp_pass":    "base64(IV||AES256(app_password))",
  "to_user":      "personal@email.com",
  "to_admin":     "admin@company.com"
}
```

### 5.4 profile.json
```json
{
  "email":         "user@email.com",
  "password_hash": "base64(PBKDF2(web_password, salt, 10000))",
  "name":          "base64(IV||AES256(full_name))",
  "dob":           "base64(IV||AES256(YYYY-MM-DD))",
  "address":       "base64(IV||AES256(address))",
  "phone":         "base64(IV||AES256(+966XXXXXXXXX))",
  "company":       "base64(IV||AES256(company_name))"
}
```

---

## 6. LittleFS Partition Layout

```
Flash 16MB layout (partition table):
┌─────────────┬──────────┬───────────────────────────┐
│ Partition   │ Size     │ Contents                  │
├─────────────┼──────────┼───────────────────────────┤
│ nvs         │ 0x5000   │ NVS (WiFi creds etc)      │
│ otadata     │ 0x2000   │ OTA data                  │
│ app0        │ 0x100000 │ Firmware (1MB)            │
│ app1        │ 0x100000 │ OTA firmware (1MB)        │
│ spiffs      │ 0xD00000 │ LittleFS (13MB)           │
└─────────────┴──────────┴───────────────────────────┘

LittleFS file tree:
/
├── config.json          ← Device settings + PIN hash + PBKDF2 salt
├── vault.json           ← Encrypted credentials array
├── network.json         ← WiFi + SMTP (fields encrypted)
├── profile.json         ← User profile (fields encrypted)
└── web/
    ├── index.html       ← Registration + Login captive portal
    └── wifi_setup.html  ← WiFi + Email configuration portal
```

---

## 7. API Endpoints (ESP32 Web Server)

### Phase 1 AP: Registration / Restore

| Method | Path | Description | Auth |
|---|---|---|---|
| GET | / | Redirect to /web/index.html | None |
| GET | /web/index.html | Serve registration/login page | None |
| POST | /api/register | Create new user account | None |
| POST | /api/restore | Login + optional vault restore | None |
| GET | /api/status | Returns setup phase and device info | None |

#### POST /api/register — Request Body
```json
{
  "email":    "user@example.com",
  "password": "min8chars",
  "name":     "Full Name",
  "dob":      "1990-01-15",
  "address":  "123 Main St, City",
  "phone":    "+966501234567",
  "company":  "Optional Inc"
}
```
#### POST /api/register — Response
```json
{ "success": true, "message": "Account created. Continue on device." }
```

#### POST /api/restore — Request (multipart/form-data)
```
email=user@example.com
password=mypassword
vault=[optional .vault file upload]
```
#### POST /api/restore — Response
```json
{ "success": true, "vaultRestored": true }
```

---

### Phase 2 AP: Network + Email Setup

| Method | Path | Description | Auth |
|---|---|---|---|
| GET | / | Redirect to /web/wifi_setup.html | AP password |
| GET | /web/wifi_setup.html | Serve WiFi setup page | AP password |
| POST | /api/test-email | Test SMTP connection | AP password |
| POST | /api/save-network | Save WiFi + email config | AP password |

#### POST /api/test-email — Request Body
```json
{
  "smtp_server": "smtp.gmail.com",
  "smtp_port":   587,
  "smtp_email":  "user@gmail.com",
  "smtp_pass":   "app_password",
  "to_user":     "personal@email.com",
  "to_admin":    "admin@company.com"
}
```

#### POST /api/save-network — Request Body
```json
{
  "wifi_ssid":    "HomeNetwork",
  "wifi_pass":    "wifipassword",
  "smtp_server":  "smtp.gmail.com",
  "smtp_port":    587,
  "smtp_email":   "user@gmail.com",
  "smtp_pass":    "app_password",
  "to_user":      "personal@email.com",
  "to_admin":     "admin@company.com"
}
```

---

## 8. UI State Machine

### 8.1 State Enum
```cpp
enum class AppState : uint8_t {
  // Boot
  BOOT_SPLASH,

  // First-boot web phase
  FIRST_BOOT_AP,           // AP active, waiting for web registration
  FIRST_BOOT_WEB_DONE,     // Registration complete, transitioning

  // On-device setup wizard
  WIZARD_LANGUAGE,
  WIZARD_PIN_SET,
  WIZARD_PIN_CONFIRM,
  WIZARD_FP_ENROLL,        // Scanning (step 1 and 2)
  WIZARD_FP_ANOTHER,       // "Add another fingerprint?"
  WIZARD_BACKUP_FREQ,

  // WiFi + email setup phase
  WIFI_EMAIL_AP,           // Second AP broadcast
  WIFI_CONNECTING,         // Testing + connecting to home WiFi

  // Normal operation
  LOCKED_IDLE,
  UNLOCK_FINGERPRINT,
  UNLOCK_PIN,
  MAIN_MENU,

  // Credentials
  CREDENTIALS_LIST,
  CREDENTIAL_DETAIL,
  CREDENTIAL_SEND,
  CREDENTIAL_SENDING,

  // Password generator
  PW_GEN_OPTIONS,
  PW_GEN_RESULT,

  // Settings
  SETTINGS_MENU,
  SETTINGS_LANGUAGE,
  SETTINGS_HID_MODE,
  SETTINGS_AUTOLOCK,
  SETTINGS_FP_MANAGE,
  SETTINGS_BACKUP_NOW,
  SETTINGS_ABOUT,

  // Overlays
  BATTERY_CRITICAL
};
```

### 8.2 Joystick Navigation Map

| Input | Action |
|---|---|
| Y-axis UP | Scroll up / increment digit |
| Y-axis DOWN | Scroll down / decrement digit |
| X-axis RIGHT | Select / confirm / advance |
| X-axis LEFT | Back / cancel |
| Button PRESS | Select / confirm (same as RIGHT) |
| Any input | Reset 30s auto-lock watchdog |

### 8.3 Key State Transitions

```
BOOT_SPLASH
  └─ [isFirstBoot] ──► FIRST_BOOT_AP
  └─ [setupDone]   ──► LOCKED_IDLE

LOCKED_IDLE
  └─ [touch wakeup / joystick] ──► UNLOCK_FINGERPRINT
  └─ [button press]             ──► UNLOCK_PIN

UNLOCK_FINGERPRINT
  └─ [OK]      ──► MAIN_MENU
  └─ [FAIL×3]  ──► LOCKED_IDLE
  └─ [LEFT]    ──► UNLOCK_PIN

UNLOCK_PIN
  └─ [correct] ──► MAIN_MENU
  └─ [wrong×5] ──► LOCKED_IDLE (penalty delay)

MAIN_MENU
  └─ item 0: Credentials  ──► CREDENTIALS_LIST
  └─ item 1: PW Generator ──► PW_GEN_OPTIONS
  └─ item 2: Settings     ──► SETTINGS_MENU
  └─ item 3: Backup Now   ──► SETTINGS_BACKUP_NOW
  └─ item 4: Lock         ──► LOCKED_IDLE
  └─ [30s watchdog]       ──► LOCKED_IDLE

CREDENTIAL_SEND
  └─ [PRESS] ──► CREDENTIAL_SENDING ──► CREDENTIAL_DETAIL

Any active state
  └─ [30s inactivity] ──► LOCKED_IDLE
  └─ [battery < 3.4V] ──► overlay BATTERY_CRITICAL
```

---

## 9. Folder Structure

```
firmware/
├── firmware.ino              ← Main sketch: setup(), loop()
├── config.h                  ← All pin defs, constants, build flags
│
├── hal/                      ← Hardware Abstraction Layer
│   ├── Display.h / .cpp      ← U8g2 SH1107 wrapper (landscape)
│   ├── Joystick.h / .cpp     ← ADC axes + button, returns JoyEvent
│   ├── Fingerprint.h / .cpp  ← GROW R503 UART2 driver
│   └── Battery.h / .cpp      ← ADC voltage divider reader
│
├── core/                     ← Core Services
│   ├── Security.h / .cpp     ← AES-256-CBC + PBKDF2 (mbedTLS)
│   ├── Storage.h / .cpp      ← LittleFS CRUD + atomic writes
│   └── Language.h / .cpp     ← 6-language string tables (PROGMEM)
│
├── app/                      ← Application Logic
│   ├── UI.h / .cpp           ← AppState FSM, dispatch, render
│   ├── PasswordManager.h/.cpp← Credential list, select, HID dispatch
│   └── PasswordGenerator.h/.cpp ← esp_random() + charset config
│
└── comm/                     ← Communication
    ├── HID_Manager.h / .cpp  ← USB HID + BLE HID unified interface
    ├── WiFiManager.h / .cpp  ← AP setup, captive portal, STA mode
    └── EmailManager.h / .cpp ← SMTP backup via ESP32-Mail-Client

web/ (uploaded to LittleFS /web/)
├── index.html                ← Registration + Restore portal
└── wifi_setup.html           ← WiFi + Email configuration portal

docs/
├── PRD.md
├── TECHNICAL_SPEC.md
├── ROADMAP.md
└── partitions.csv
```

---

## 10. Custom Partition Table (partitions.csv)

```csv
# Name,   Type, SubType, Offset,    Size,     Flags
nvs,      data, nvs,     0x9000,    0x5000,
otadata,  data, ota,     0xe000,    0x2000,
app0,     app,  ota_0,   0x10000,   0x100000,
app1,     app,  ota_1,   0x110000,  0x100000,
spiffs,   data, spiffs,  0x210000,  0xDF0000,
```

---

## 11. Build & Flash Instructions

### 11.1 Arduino IDE Setup
1. Install Arduino IDE 2.x
2. Add ESP32 board URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Install **esp32 by Espressif** ≥ 2.0.14
4. Board: **ESP32S3 Dev Module**
5. Settings:
   - Flash Size: **16MB (128Mb)**
   - Partition Scheme: **Custom** (use `partitions.csv`)
   - PSRAM: **OPI PSRAM**
   - USB Mode: **USB-OTG (TinyUSB)**
   - USB CDC On Boot: **Enabled**

### 11.2 Required Libraries (Library Manager)
```
U8g2                        by olikraus
Adafruit Fingerprint Sensor by Adafruit
NimBLE-Arduino              by h2zero
ArduinoJson                 by Benoit Blanchon   v7.x
ESP32-Mail-Client           by mobizt
```

### 11.3 Manual Install (GitHub)
```bash
# ESPAsyncWebServer
git clone https://github.com/me-no-dev/ESPAsyncWebServer.git
# AsyncTCP  
git clone https://github.com/me-no-dev/AsyncTCP.git
# Copy both into ~/Arduino/libraries/
```

### 11.4 Upload Web Assets (LittleFS)
1. Install LittleFS upload plugin for Arduino IDE
2. Place `index.html` and `wifi_setup.html` in `firmware/data/web/`
3. Tools → ESP32 LittleFS Data Upload

### 11.5 Flash Commands (esptool)
```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 \
  write_flash 0x0 bootloader.bin \
             0x8000 partitions.bin \
             0x10000 firmware.bin
```

---

## 12. Edge Cases & Error Handling

| Scenario | Handling |
|---|---|
| Battery below 3.4V during setup | Show warning overlay; allow completion |
| Battery below 3.0V | Force lock + warn; disable HID output |
| R503 sensor not detected at boot | Show error; fallback to PIN-only mode |
| R503 password mismatch (tampered) | Emergency lock; require factory reset |
| Wrong PIN 5 times | 30-second lockout penalty + counter increment |
| vault.json corrupted | Load from vault.json.tmp if exists; else empty vault |
| WiFi connection fails | 3 retries; skip backup this cycle; retry next cycle |
| SMTP send fails | Log failure; retry on next backup cycle |
| LittleFS full (>90%) | Warn user; suggest deleting old credentials |
| BLE disconnected during typing | Cancel send; notify user |
| USB unplugged during typing | Cancel send gracefully |
| Joystick ADC floating values | 10% deadzone filter + hysteresis |
| OLED I2C not responding | Boot without display; PIN-only via serial debug |
| Deep sleep wakeup (non-touch) | Check wakeup cause; if not EXT0, stay locked |
| Power cycle during LittleFS write | Atomic rename prevents corruption |
| Backup file > email attachment limit | Chunk or compress vault blob |
