# Corrected Setup Flow — Final Version

## Phase 1 — Web Registration (AP Broadcast #1)

```
Device powers on → config.json not found → First Boot
        │
        ▼
Broadcasts "PwdMgr_Setup" (WPA2)
AP password: "Pm@9Secure!26x"
  → Min 10 chars ✓ | Uppercase ✓ | Lowercase ✓ | Digit ✓ | Symbol ✓
  → Hardcoded in firmware | Same for all devices | Cannot change
        │
        ▼
User opens  http://192.168.4.1  on their phone

Registration form fields:
  ── Account credentials ──
  ✓ Email address
  ✓ Password (min 8 chars: A-Z + a-z + 0-9 + symbol required)
  ✓ Confirm password
  ── Personal information ──
  ✓ Full name
  ✓ Date of birth
  ✓ Phone + country code
  ✓ Company name (OPTIONAL)
  ── Admin settings ──
  ✓ Admin email (required — receives encrypted backup copy)
        │
        ▼
POST /api/register → ESP32 saves to profile.json + network.json
        │
        ▼
Page shows: "Account Created! Follow steps on device screen."
Steps shown on success page:
  1. Select display language
  2. Set secure device PIN (8+ chars, A-Z a-z 0-9 symbol)
  3. Enroll 1-5 fingerprints
  4. Reconnect to PwdMgr_Setup for WiFi & email setup

AP closes automatically after successful registration.
```

---

## Phase 2 — On-Device Wizard

```
WIZARD_LANGUAGE
  │ ↑↓ joystick selects language, ► confirms
  ▼
WIZARD_PIN_SET        ← CharInput alphanumeric (NOT digit boxes)
  │
  │ PIN requirements enforced live on OLED:
  │   ● A-Z  ● a-z  ● 0-9  ● !symbol  ● 8+ chars
  │   (filled dot = requirement met)
  │ hold ► to finish entry
  ▼
WIZARD_PIN_CONFIRM    ← Retype same PIN to confirm
  │ Mismatch → error screen → restart from PIN_SET
  ▼
WIZARD_FP_ENROLL      ← Scan finger TWICE per fingerprint
  │ R503 LED: Blue=scan 1 → Purple=scan 2 → Green=saved
  ▼
WIZARD_FP_ANOTHER     ← "Add another fingerprint? YES / NO"
  │ YES → back to WIZARD_FP_ENROLL (up to 5 total)
  │ NO  →
  ▼
Wizard complete → trigger Phase 3 AP
```

---

## Phase 3 — WiFi & Email Setup (AP Broadcast #2)

```
Device broadcasts "PwdMgr_Setup" again (same credentials)
OLED shows: "Connect to PwdMgr_Setup / Open 192.168.4.1"
        │
        ▼
wifi_setup.html loads on user's phone
(Admin email already saved from Phase 1 — pre-filled)

Fields:
  ── WiFi Network ──
  ✓ Home/Office WiFi SSID
  ✓ WiFi password
  ── Email sender account ──
  ✓ Email provider preset (Gmail/Outlook/Yahoo/iCloud/Custom)
  ✓ Sender email + app password
  ── Backup recipients ──
  ✓ Personal inbox email (user receives their backup)
  ✓ Admin email (pre-filled from registration, editable)
        │
        ▼
Test SMTP → Save → Device connects to home WiFi → AP closes
OLED: "✓ Setup Complete! Touch sensor to unlock."
        │
        ▼
→ LOCKED_IDLE (normal operation begins)
```

---

## Device PIN Requirements

| Rule | Value |
|---|---|
| Minimum length | 8 characters |
| Maximum length | 64 characters |
| Uppercase letters (A-Z) | At least 1 |
| Lowercase letters (a-z) | At least 1 |
| Digits (0-9) | At least 1 |
| Symbols (!@#$%^&*) | At least 1 |
| Character set | ASCII / English only |
| Entry method | Joystick CharInput (cycling A→Z→a→z→0→9→symbols) |
| Display | Each char shown as * after confirmation |
| Strength indicator | Live dots on OLED for each requirement |

---

## Setup AP Password Requirements

| Rule | Value |
|---|---|
| Minimum length | 10 characters |
| Current value | `Pm@9Secure!26x` (14 chars) |
| Uppercase | P ✓ |
| Lowercase | m, ecure, x ✓ |
| Digit | 9, 26 ✓ |
| Symbol | @, ! ✓ |
| Configurable by user | NO — hardcoded in config.h |
| Configurable by admin | NO — requires firmware recompile |
| Per-device unique | NO — same for all devices of this build |

---

## What Changed vs Previous Version

| Item | Old | Corrected |
|---|---|---|
| Company name | On device (joystick) — WRONG | Web form, Phase 1 registration ✓ |
| Admin email | On device or WiFi page | Web form (Phase 1) — pre-filled in Phase 3 |
| Device PIN | 4-6 digit boxes | 8+ alphanumeric via CharInput |
| PIN validation | None | Live: A-Z, a-z, 0-9, symbol, 8+ chars |
| AP password req. | Not documented | Min 10, upper+lower+digit+symbol |
| On-device wizard | Language→PIN→FP→company→admin | Language→PIN→FP only |
