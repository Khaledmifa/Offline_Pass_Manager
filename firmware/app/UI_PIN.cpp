// =============================================================
//  UI_PIN.cpp — Corrected PIN wizard states
//  Replaces the old digit-box PIN entry with alphanumeric CharInput.
//  Merge into UI.cpp in the final build.
//
//  PIN rules (enforced here and in Security::validatePINStrength):
//    ✓ Minimum 8 characters
//    ✓ At least 1 uppercase  (A-Z)
//    ✓ At least 1 lowercase  (a-z)
//    ✓ At least 1 digit      (0-9)
//    ✓ At least 1 symbol     (!@#$%^&*-_+=)
//    ✓ ASCII / English only
// =============================================================

#include "UI.h"
#include "CharInput.h"
#include "../hal/Display.h"
#include "../core/Security.h"
#include "../core/Storage.h"

// PIN stored across step 1 and step 2
static char gWizardPIN[PIN_MAX_LEN + 1] = {};
static bool gPINConfirmMode             = false;
static bool gPINMismatch                = false;
static bool gPINWeak                    = false;

// ── Enter WIZARD_PIN_SET ──────────────────────────────────────
void UI_beginPINSet() {
  memset(gWizardPIN,  0, sizeof(gWizardPIN));
  gPINConfirmMode = false;
  gPINMismatch    = false;
  gPINWeak        = false;
  CharInput::begin("Set Device PIN", PIN_MAX_LEN, false);
}

// ── Input handler (call from UI::tick when state == WIZARD_PIN_SET) ──
// Returns true when the PIN has been confirmed and saved.
bool UI_handlePINWizard(JoyEvent e) {

  // ── Error flash cooldown ──────────────────────────────────
  if (gPINMismatch || gPINWeak) {
    if (e != JoyEvent::NONE) {
      gPINMismatch = false;
      gPINWeak     = false;
      if (gPINConfirmMode) {
        // Re-enter confirm
        CharInput::begin("Confirm PIN", PIN_MAX_LEN, false);
      } else {
        CharInput::begin("Set Device PIN", PIN_MAX_LEN, false);
      }
    }
    return false;
  }

  bool done = CharInput::feed(e);

  if (done) {
    String entered = CharInput::result();

    if (!gPINConfirmMode) {
      // ── Step 1: validate strength ───────────────────────
      if (!Security::validatePINStrength(entered.c_str(), entered.length())) {
        gPINWeak = true;
        return false;
      }
      // Store and move to confirm
      strncpy(gWizardPIN, entered.c_str(), PIN_MAX_LEN);
      gPINConfirmMode = true;
      CharInput::begin("Confirm PIN", PIN_MAX_LEN, false);

    } else {
      // ── Step 2: verify match ────────────────────────────
      if (strcmp(gWizardPIN, entered.c_str()) != 0) {
        gPINMismatch    = true;
        gPINConfirmMode = false;
        memset(gWizardPIN, 0, sizeof(gWizardPIN));
        return false;
      }
      // ── Match — hash and save ───────────────────────────
      Security::generateSalt();       // generate PBKDF2 salt
      DeviceConfig cfg;
      Storage::loadConfig(cfg);
      cfg.pinHash = Security::hashPIN(gWizardPIN, strlen(gWizardPIN));
      Storage::saveConfig(cfg);
      // Zero sensitive buffer immediately
      memset(gWizardPIN, 0, sizeof(gWizardPIN));
      return true;   // PIN wizard complete
    }
  }
  return false;
}

// ── Renderer ─────────────────────────────────────────────────
void UI_renderPINWizard() {
  if (gPINWeak) {
    Display::drawMessage(
      "PIN too weak",
      "Need: A-Z a-z 0-9 !symbol  min 8 chars",
      true);
    return;
  }
  if (gPINMismatch) {
    Display::drawMessage(
      "PIN mismatch",
      "PINs did not match. Try again.",
      true);
    return;
  }
  bool strengthOk = Security::validatePINStrength(
    CharInput::state().buf, CharInput::state().bufLen);
  Display::drawPINEntryAlpha(CharInput::state(),
                               gPINConfirmMode,
                               false,
                               strengthOk);
}

// ── Unlock PIN (normal use, same alphanumeric rules) ──────────
static char gUnlockBuf[PIN_MAX_LEN + 1] = {};
static bool gUnlockActive                = false;

void UI_beginUnlockPIN() {
  memset(gUnlockBuf, 0, sizeof(gUnlockBuf));
  gUnlockActive = true;
  CharInput::begin("Enter Device PIN", PIN_MAX_LEN, false);
}

// Returns 1 = correct, -1 = wrong, 0 = still entering
int UI_handleUnlockPIN(JoyEvent e, uint8_t& wrongCount,
                        const String& storedHash) {
  bool done = CharInput::feed(e);
  if (!done) return 0;

  String entered = CharInput::result();
  if (Security::verifyPIN(entered.c_str(), entered.length(), storedHash)) {
    // Derive AES master key from PIN
    Security::deriveKey(entered.c_str(), entered.length());
    wrongCount   = 0;
    gUnlockActive = false;
    return 1;   // correct
  }
  wrongCount++;
  CharInput::begin("Enter Device PIN", PIN_MAX_LEN, false);
  return -1;    // wrong
}

void UI_renderUnlockPIN(uint8_t wrongCount) {
  Display::drawPINEntryAlpha(CharInput::state(), false,
                               wrongCount > 0, false);
}
