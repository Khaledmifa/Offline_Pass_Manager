# Hardware Password Manager — Product Requirements Document (PRD)
**Version:** 1.0.0 | **Date:** 2025 | **Status:** Draft → Ready for Development

---

## 1. Executive Summary | الملخص التنفيذي

A standalone, offline-first hardware device that stores, encrypts, and auto-types credentials via USB or Bluetooth. The device requires biometric or PIN authentication before any credential can be accessed. All data is encrypted with AES-256 and stored locally; encrypted backups are emailed automatically on a configurable schedule.

جهاز مستقل يعمل دون إنترنت يقوم بتخزين وتشفير وإدخال كلمات المرور تلقائياً عبر USB أو Bluetooth. يتطلب الجهاز مصادقة بيومترية أو رمز PIN قبل الوصول إلى أي بيانات. جميع البيانات مشفرة بـ AES-256 ومخزنة محلياً مع نسخ احتياطية مشفرة ترسل بالبريد الإلكتروني تلقائياً.

---

## 2. Problem Statement | المشكلة

| Problem | Impact |
|---|---|
| Passwords stored in browsers or apps are vulnerable to malware | High |
| Cloud password managers require internet and trust in a third party | High |
| Hardware security keys (YubiKey etc.) don't store passwords, only authenticate | Medium |
| Writing passwords down is insecure | High |
| Reusing passwords across sites | Critical |

**Solution:** A self-contained hardware vault that never exposes plaintext credentials to any host computer — it simply acts as a keyboard and types the password for you.

---

## 3. Target Users | المستخدمون المستهدفون

| Segment | Description |
|---|---|
| **Primary** | Privacy-conscious individuals who want offline credential management |
| **Secondary** | IT professionals needing a portable, hardware-secured credential store |
| **Enterprise** | Small teams where an admin manages device fleet and receives encrypted backup copies |

---

## 4. Functional Requirements | المتطلبات الوظيفية

### 4.1 Authentication
| ID | Requirement | Priority |
|---|---|---|
| FR-01 | Device locks by default on power-on | Must |
| FR-02 | Unlock via GROW R503 fingerprint sensor | Must |
| FR-03 | Fallback unlock via 4–6 digit PIN (joystick entry) | Must |
| FR-04 | Auto-lock after 30 seconds of inactivity (configurable) | Must |
| FR-05 | Lock device after 5 consecutive wrong PIN attempts | Must |
| FR-06 | Support up to 5 enrolled fingerprints | Must |
| FR-07 | Touch sensor wakes device from deep sleep (EXT0 interrupt) | Must |

### 4.2 Credential Management
| ID | Requirement | Priority |
|---|---|---|
| FR-10 | Store up to 200 credentials (name, username, password) | Must |
| FR-11 | Auto-type via USB HID keyboard | Must |
| FR-12 | Auto-type via BLE HID keyboard | Must |
| FR-13 | Choose send mode: Username+Password / Username only / Password only | Must |
| FR-14 | Tab key inserted between username and password fields | Must |
| FR-15 | Add, edit, delete credentials | Must |

### 4.3 Password Generator
| ID | Requirement | Priority |
|---|---|---|
| FR-20 | Generate cryptographically random passwords (hardware TRNG) | Must |
| FR-21 | Configurable length 8–64 characters | Must |
| FR-22 | Configurable charset: uppercase, lowercase, digits, symbols | Must |
| FR-23 | Preview generated password on OLED (horizontal scroll) | Must |
| FR-24 | Save generated password directly to a new credential entry | Must |

### 4.4 First-Boot Setup — Web Phase
| ID | Requirement | Priority |
|---|---|---|
| FR-30 | Device broadcasts WPA2-protected AP "PwdMgr_Setup" on first boot | Must |
| FR-31 | AP password is hardcoded in firmware, cannot be changed | Must |
| FR-32 | All connected clients redirected to captive portal (DNS redirect) | Must |
| FR-33 | Portal served from LittleFS — no internet required | Must |
| FR-34 | New account registration: email, password, name, DOB, address, phone+country, company (opt.) | Must |
| FR-35 | Restore backup: email + password login, optional .vault file upload | Must |
| FR-36 | After web step, AP closes and on-device wizard begins | Must |

### 4.5 First-Boot Setup — On-Device Wizard
| ID | Requirement | Priority |
|---|---|---|
| FR-40 | Step 1: Language selection (EN, AR, DE, FR, TR, ZH) | Must |
| FR-41 | Step 2: Set Master PIN (4–6 digits via joystick) | Must |
| FR-42 | Step 3: Confirm Master PIN | Must |
| FR-43 | Step 4: Enroll at least one fingerprint (two scans each) | Must |
| FR-44 | Option to enroll additional fingerprints up to max 5 | Must |
| FR-45 | Step 5: Choose backup frequency (1 day / 3 days / 1 week / 2 weeks / 1 month) | Must |
| FR-46 | Setup complete confirmation screen | Must |

### 4.6 WiFi & Email Configuration (Second AP)
| ID | Requirement | Priority |
|---|---|---|
| FR-50 | After on-device wizard, device broadcasts AP again | Must |
| FR-51 | User enters: home WiFi SSID + password | Must |
| FR-52 | User enters: SMTP credentials (server, port, email, password) | Must |
| FR-53 | Gmail / Outlook / Yahoo / iCloud presets auto-fill SMTP settings | Should |
| FR-54 | User enters: personal inbox email (backup recipient) | Must |
| FR-55 | User enters: admin email (backup recipient) | Must |
| FR-56 | Test SMTP connection from configuration page | Should |
| FR-57 | Admin authenticates via AP WPA2 password (no separate credential) | Must |
| FR-58 | Admin password = setup AP password, hardcoded, cannot change | Must |

### 4.7 Backup System
| ID | Requirement | Priority |
|---|---|---|
| FR-60 | Encrypted vault blob sent to user email on schedule | Must |
| FR-61 | Encrypted copy simultaneously sent to admin email | Must |
| FR-62 | Vault is AES-256 encrypted before transmission; never plaintext | Must |
| FR-63 | Admin copy is unreadable without user's PIN-derived key | Must |
| FR-64 | Backup includes timestamp and device ID in subject line | Should |
| FR-65 | Backup can be triggered manually from Settings menu | Should |

### 4.8 Display & UI
| ID | Requirement | Priority |
|---|---|---|
| FR-70 | SH1107 OLED rotated 90° in software for landscape (128×64) | Must |
| FR-71 | Battery percentage displayed in status bar | Must |
| FR-72 | HID mode (USB/BLE) displayed in status bar | Must |
| FR-73 | Smooth joystick menu navigation: ↑↓ scroll, ► select, ◄ back | Must |
| FR-74 | Auto-dim OLED after 60 seconds of inactivity | Should |
| FR-75 | OLED off after device locks | Must |

### 4.9 Multi-Language Support
| ID | Requirement | Priority |
|---|---|---|
| FR-80 | UI strings in: English, Arabic, German, French, Turkish, Chinese | Must |
| FR-81 | Arabic: right-to-left text alignment | Must |
| FR-82 | Language selectable from Settings menu anytime | Must |

---

## 5. Non-Functional Requirements | المتطلبات غير الوظيفية

| ID | Requirement | Target |
|---|---|---|
| NFR-01 | Master key never written to flash storage | Always |
| NFR-02 | Key derived via PBKDF2-SHA256, 100,000 iterations | Always |
| NFR-03 | AES-256-CBC with random IV per credential | Always |
| NFR-04 | R503 fingerprint sensor hardware password enabled | Always |
| NFR-05 | Setup AP WPA2 password hardcoded, not user-configurable | Always |
| NFR-06 | Auto-lock max timeout: 5 minutes | Always |
| NFR-07 | Battery life: minimum 4 hours active use (800mAh LiPo) | Target |
| NFR-08 | Boot to locked screen: < 3 seconds | Target |
| NFR-09 | Fingerprint verification: < 2 seconds | Target |
| NFR-10 | HID typing speed: 10ms inter-key delay (adjustable) | Target |
| NFR-11 | LittleFS atomic writes prevent vault corruption on power-loss | Always |

---

## 6. User Stories | قصص المستخدم

```
As a user, I want to touch the fingerprint sensor to wake and unlock the device
  so that I can access my credentials without touching any buttons.

As a user, I want to scroll through my credential list with the joystick
  and press to auto-type my password into the focused login field.

As a user, I want to choose whether to type only username, only password,
  or both, so I can handle multi-step login pages.

As a user, I want to generate a strong random password and immediately
  save it to a new account entry without writing it down.

As an admin, I want to receive an encrypted backup copy of the user's vault
  by email, so I can assist with device recovery if needed.

As a user, I want to replace my device (or factory-reset it) and restore
  my vault by logging in via the setup portal and uploading my backup file.
```

---

## 7. Out of Scope (v1.0)

- Cloud-hosted web dashboard
- Multi-user / team sharing
- TOTP / 2FA code generation (future v1.5)
- NFC output (future)
- Desktop companion app
- FIDO2 / WebAuthn (future)

---

## 8. Success Metrics

| Metric | Target |
|---|---|
| Setup completion time (first boot to ready) | < 5 minutes |
| Credential auto-type reliability | > 99% on standard login fields |
| Battery life (active use) | > 4 hours |
| Fingerprint false acceptance rate | < 0.001% (R503 spec) |
| Vault recovery from backup | 100% data integrity |
