# Corrections & Design Decisions — Final Record

This document records every change made from the initial design
to the final agreed architecture. Use it as the single source of truth
when something seems inconsistent across other files.

---

## 1. Registration Form (index.html) — Final Fields

All of the following are collected on the **web registration page**
during Phase 1 (first AP broadcast). Nothing is entered on the device
except language, PIN, and fingerprints.

```
Section 1 — Account credentials
  ✓ Email address
  ✓ Password           (min 8 chars, A-Z + a-z + 0-9 + symbol required)
  ✓ Confirm password

Section 2 — Personal information
  ✓ Full name
  ✓ Date of birth
  ✓ Phone + country code
  ✓ Company / Organisation   (OPTIONAL — user may leave blank)

Section 3 — Admin settings
  ✓ Admin email   (REQUIRED — receives encrypted backup copy)
```

**Nothing from this list is entered on the device via joystick.**

---

## 2. Setup Flow — Final Agreed Order

```
┌─────────────────────────────────────────────────────────────────┐
│  PHASE 1 — Web Registration (AP Broadcast #1)                   │
│  Device broadcasts "PwdMgr_Setup"                               │
│  User opens http://192.168.4.1 on phone                         │
│  Enters: email, password, confirm, name, DOB, phone,            │
│          company (optional), admin email                         │
│  Saves to device → page shows "Continue on device screen"        │
│  AP closes automatically                                         │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│  PHASE 2 — On-Device Wizard (joystick only)                     │
│  Step 1: Language selection                                      │
│  Step 2: Set Device PIN (8+ alphanumeric chars, strength check)  │
│  Step 3: Confirm PIN (retype to verify match)                    │
│  Step 4: Enroll fingerprint (two scans, saves to R503)           │
│  Step 5: "Add another fingerprint?" YES/NO — up to 5 total      │
│  Device shows: "Connect to PwdMgr_Setup to finish setup"         │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│  PHASE 3 — WiFi & Email Setup (AP Broadcast #2)                 │
│  Device broadcasts "PwdMgr_Setup" again (same password)          │
│  User opens http://192.168.4.1 on phone → wifi_setup.html       │
│  Enters:                                                         │
│    - Home WiFi SSID + password                                   │
│    - SMTP server preset + email + app password                   │
│    - Personal inbox (backup recipient)                           │
│    - Admin email (pre-filled from Phase 1, editable)             │
│  Device connects to home WiFi → AP closes → LOCKED_IDLE          │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. Device PIN — Changed from Digits to Alphanumeric

| Old design (wrong) | Final design (correct) |
|---|---|
| 4–6 digit boxes | 8–64 character alphanumeric passphrase |
| Only 0–9 digits | A-Z + a-z + 0-9 + symbols (!@#$%) |
| Digit spinner UI | CharInput joystick cycling system |
| No strength check | Live strength indicator on OLED |
| Simple storage | PBKDF2-SHA256 (100,000 rounds) + AES-256 key derivation |

PIN entry on OLED shows a live strength indicator:
```
A-Z:● a-z:● 0-9:○ !:○   (filled dot = requirement met)
```

---

## 4. Setup AP Password — Requirements Documented

| Property | Value |
|---|---|
| Current password | `Pm@9Secure!26x` |
| Minimum length | 10 characters (current: 14) |
| Uppercase (A-Z) | Required — P ✓ |
| Lowercase (a-z) | Required — m, ecure, x ✓ |
| Digit (0-9) | Required — 9, 2, 6 ✓ |
| Symbol | Required — @, ! ✓ |
| Configurable by user | NO |
| Configurable by admin | NO |
| How to change | Edit SETUP_AP_PASS in config.h → recompile firmware |
| Per-device unique | NO — same for all devices of this build |

---

## 5. New Features Added

**Smart site name autocomplete (`SiteDatabase.h`):**
When adding a credential, the user types the site name using the
joystick. With each character entered, the device narrows the
suggestions from ~260 pre-loaded site names stored in flash.
If the site is not in the database, the user types the full name.

**Joystick text entry (`CharInput.h/.cpp`):**
Reusable character-by-character text input for all on-device text
fields. Charset: A-Z, a-z, 0-9, and symbols. Used for PIN entry,
site names, usernames, and passwords.

---

## 6. What Is Entered Where — Quick Reference

| Information | Entered where |
|---|---|
| Email, Password, Name, DOB, Phone | Web page (Phase 1) |
| Company name (optional) | Web page (Phase 1) |
| Admin email | Web page (Phase 1) |
| Display language | Device joystick (Phase 2) |
| Device PIN (8+ alphanumeric) | Device joystick (Phase 2) |
| Fingerprints (1–5) | R503 sensor (Phase 2) |
| Home WiFi SSID + password | Web page (Phase 3) |
| SMTP credentials | Web page (Phase 3) |
| Personal inbox email | Web page (Phase 3) |
| Admin email confirmation | Web page (Phase 3, pre-filled) |
