#pragma once
// =============================================================
//  Hardware Password Manager — ESP32-S3 (N16R8)
//  config.h  |  All pin definitions, constants, build settings
// =============================================================

// ── Firmware ─────────────────────────────────────────────────
#define FW_VERSION          "1.0.0"
#define DEVICE_NAME         "PwdMgr"

// ── I2C (SH1107 OLED 64x128, landscape via U8G2_R1) ──────────
#define OLED_SDA_PIN        8
#define OLED_SCL_PIN        9

// ── UART2 (GROW R503 fingerprint) ─────────────────────────────
#define FP_RX_PIN           18
#define FP_TX_PIN           17
#define FP_BAUD             57600
#define FP_WAKEUP_PIN       GPIO_NUM_5    // R503 WAKEUP → EXT0 deep-sleep

// ── Joystick ──────────────────────────────────────────────────
#define JOY_X_PIN           1             // ADC1_CH0
#define JOY_Y_PIN           2             // ADC1_CH1
#define JOY_BTN_PIN         3             // INPUT_PULLUP (active LOW)
#define JOY_ADC_MAX         4095
#define JOY_CENTRE          2048
#define JOY_DEADZONE        410           // 10% of 4095
#define JOY_REPEAT_MS       150           // held-direction repeat interval
#define JOY_DEBOUNCE_MS     20

// ── Battery (100K/100K voltage divider) ───────────────────────
#define BAT_ADC_PIN         4             // ADC1_CH3
#define BAT_ADC_SAMPLES     16
#define BAT_VREF_MV         3300
#define BAT_DIVIDER         2
#define BAT_FULL_MV         4200
#define BAT_EMPTY_MV        3300
#define BAT_CRITICAL_MV     3400

// ── WiFi Setup AP ─────────────────────────────────────────────
// PASSWORD REQUIREMENTS (enforced at compile-time comments):
//   ✓ Minimum 10 characters
//   ✓ At least one uppercase letter (A-Z)
//   ✓ At least one lowercase letter (a-z)
//   ✓ At least one digit (0-9)
//   ✓ At least one symbol (!@#$%^&* etc.)
//   ✓ Hardcoded — cannot be changed by user or admin
//   ✓ Same for ALL devices (not per-device)
//
// Current value: "Pm@9Secure!26x" — 14 chars, meets all requirements.
// To change: edit this define and recompile the firmware.
#define SETUP_AP_SSID       "PwdMgr_Setup"
#define SETUP_AP_PASS       "Pm@9Secure!26x"   // 14 chars | NEVER expose in logs/API
#define SETUP_AP_CHANNEL    6
#define SETUP_AP_IP         "192.168.4.1"
#define SETUP_AP_TIMEOUT_MS 1800000UL           // 30 min auto-close

// ── Security — AES-256 / PBKDF2 ──────────────────────────────
#define AES_KEY_BYTES       32
#define AES_IV_BYTES        16
#define PBKDF2_ROUNDS       100000
#define PBKDF2_SALT_BYTES   16

// ── Device PIN ────────────────────────────────────────────────
// The device PIN is an alphanumeric passphrase, NOT just digits.
// Requirements:
//   ✓ Minimum 8 characters (no maximum, up to 64)
//   ✓ At least one uppercase letter (A-Z)
//   ✓ At least one lowercase letter (a-z)
//   ✓ At least one digit (0-9)
//   ✓ At least one symbol (!@#$%^&*-_+=)
//   ✓ ASCII / English characters only
//   ✓ Entered via joystick CharInput system on OLED
#define PIN_MIN_LEN         8
#define PIN_MAX_LEN         64
#define PIN_MAX_ATTEMPTS    5             // lock after 5 consecutive wrong PINs

// ── Fingerprint ───────────────────────────────────────────────
#define FP_MAX_SLOTS        5
#define FP_HW_PASSWORD      0xA5B6C7D8UL  // R503 hardware password
#define FP_ENROLL_TIMEOUT   8000
#define FP_VERIFY_TIMEOUT   5000

// ── LittleFS paths ────────────────────────────────────────────
#define PATH_VAULT          "/vault.json"
#define PATH_CONFIG         "/config.json"
#define PATH_NETWORK        "/network.json"
#define PATH_PROFILE        "/profile.json"
#define PATH_WEB_INDEX      "/web/index.html"
#define PATH_WEB_WIFI       "/web/wifi_setup.html"

// ── Auto-lock ─────────────────────────────────────────────────
#define AUTOLOCK_DEFAULT_MS 30000UL
#define OLED_OFF_DELAY_MS   5000UL

// ── HID output ────────────────────────────────────────────────
#define HID_KEY_DELAY_MS    10
#define HID_FIELD_DELAY_MS  100

// ── Password generator ────────────────────────────────────────
#define PWGEN_MIN           8
#define PWGEN_MAX           64
#define PWGEN_DEFAULT       16

// ── OLED layout ───────────────────────────────────────────────
#define OLED_W              128           // after U8G2_R1 rotation
#define OLED_H              64
#define STATUSBAR_H         11
#define CONTENT_Y           (STATUSBAR_H + 1)
#define CONTENT_H           (OLED_H - CONTENT_Y)
