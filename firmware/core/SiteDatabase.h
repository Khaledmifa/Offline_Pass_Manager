#pragma once
#include <Arduino.h>

// =============================================================
//  SiteDatabase.h | Built-in site/app name database (~260 entries)
//  Stored in flash (const). Used for autocomplete when adding
//  a new credential. Search is case-insensitive prefix match.
// =============================================================

class SiteDatabase {
public:
  // Fill `results` with up to `maxN` site names that start with `prefix`.
  // Returns number of matches found.
  static uint8_t search(const char* prefix,
                         const char** results, uint8_t maxN);

  static uint16_t totalCount();

private:
  static const char* const _db[];
  static const uint16_t    _count;
};
