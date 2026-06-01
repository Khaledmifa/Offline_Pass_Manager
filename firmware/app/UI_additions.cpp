// =============================================================
//  UI_additions.cpp
//  New states added to UI.cpp to support:
//    1. CRED_ADD_NAME  — joystick char input + site autocomplete
//    2. CRED_ADD_USER  — joystick char input for username
//    3. CRED_ADD_PASS  — joystick char input for password (masked)
//
//  NOTE: WIZARD_ADMIN_SETUP was removed.
//  Company name and admin email are entered on the web
//  registration page (index.html, Phase 1), NOT on the device.
//  The device wizard only handles: Language, PIN, Fingerprints.
// =============================================================

#include "UI.h"
#include "CharInput.h"
#include "../hal/Display.h"
#include "../core/Storage.h"
#include "PasswordManager.h"

// ── New AppState values (add to UI.h enum) ────────────────────
//   CRED_ADD_NAME,    // typing site/app name — with autocomplete
//   CRED_ADD_USER,    // typing username
//   CRED_ADD_PASS,    // typing password (shown as ***)

// ── Buffers used across the add-credential flow ───────────────
static char gCredName[64] = {};
static char gCredUser[64] = {};
static char gCredPass[64] = {};

// ═══════════════════════════════════════════════════════════════
//  ADD CREDENTIAL — three-step flow
//  Step 1: CRED_ADD_NAME  — site/app name with autocomplete
//  Step 2: CRED_ADD_USER  — username / email address
//  Step 3: CRED_ADD_PASS  — password (characters shown as ***)
// ═══════════════════════════════════════════════════════════════

enum class AddCredStep : uint8_t { NAME, USER, PASS, SAVED };
static AddCredStep gAddStep = AddCredStep::NAME;

void UI_beginAddCredential() {
  memset(gCredName, 0, sizeof(gCredName));
  memset(gCredUser, 0, sizeof(gCredUser));
  memset(gCredPass, 0, sizeof(gCredPass));
  gAddStep = AddCredStep::NAME;
  // Enable autocomplete for site name step
  CharInput::begin("Site / App name", 40, true);
}

// Returns true when credential has been saved.
bool UI_handleAddCredential(JoyEvent e) {
  bool done = CharInput::feed(e);
  if (!done) return false;

  switch (gAddStep) {
    case AddCredStep::NAME:
      strncpy(gCredName, CharInput::result().c_str(), sizeof(gCredName) - 1);
      gAddStep = AddCredStep::USER;
      CharInput::begin("Username / Email", 64, false);
      break;

    case AddCredStep::USER:
      strncpy(gCredUser, CharInput::result().c_str(), sizeof(gCredUser) - 1);
      gAddStep = AddCredStep::PASS;
      CharInput::begin("Password", 64, false);
      break;

    case AddCredStep::PASS:
      strncpy(gCredPass, CharInput::result().c_str(), sizeof(gCredPass) - 1);
      PasswordManager::add(String(gCredName),
                            String(gCredUser),
                            String(gCredPass));
      // Zero sensitive buffers immediately after save
      memset(gCredPass, 0, sizeof(gCredPass));
      gAddStep = AddCredStep::SAVED;
      Display::drawMessage("Saved!", gCredName);
      delay(1400);
      return true;   // done — caller returns to CREDENTIALS_LIST

    default: break;
  }
  return false;
}

void UI_renderAddCredential() {
  // For password step: mask confirmed characters as *
  if (gAddStep == AddCredStep::PASS) {
    CharInputState masked = CharInput::state();
    for (uint8_t i = 0; i < masked.bufLen; i++) masked.buf[i] = '*';
    Display::drawCharInput(masked);
  } else {
    Display::drawCharInput(CharInput::state());
  }
}

bool UI_addCredentialDone() {
  return gAddStep == AddCredStep::SAVED;
}
