#include "SiteDatabase.h"
#include <string.h>

// ── Site database — sorted alphabetically ─────────────────────
// Stored as const char* in flash (ESP32 maps flash to address space)
const char* const SiteDatabase::_db[] = {
  // A
  "Absher","Adobe","Adobe Creative Cloud","AirBnb","Al Rajhi Bank",
  "Alinma Bank","AliExpress","Amazon","Amazon AWS","Amazon Prime",
  "ANB Bank","Anghami","Apple ID","Apple Music","Apple TV+",
  "Atlassian","Azure",
  // B
  "Bein Sports","Binance","Bitbucket","Bitwarden","Booking.com","Box","BSF Bank",
  // C
  "Canva","Careem","ChatGPT","Cloudflare","Coinbase","Coursera",
  // D
  "Deliveroo","Deezer","Discord","Disney+","Dropbox",
  // E
  "eBay","Epic Games","Etsy","Evernote","Extra Stores",
  // F
  "Facebook","Fawry","Figma","Fitbit","Flyadeal","Freelancer",
  // G
  "GitHub","GitLab","Gmail","Google","Google Drive","Google Meet",
  "Google Photos","Google Play","Google Workspace",
  // H
  "Hala","HBO Max","Hotmail","HSBC","HungerStation",
  // I
  "iCloud","Instagram","Instacart",
  // J
  "Jarir Bookstore","Jira",
  // K
  "Kraken","KuCoin",
  // L
  "LastPass","LinkedIn","LinkedIn Learning","Lyft",
  // M
  "Microsoft","Microsoft 365","Microsoft Azure","Microsoft Teams",
  "Mobily","MyFatoorah",
  // N
  "Namshi","Netflix","Noon","Notion","NordVPN","Nu Bank",
  // O
  "OneDrive","OpenAI","Outlook",
  // P
  "PayPal","Pinterest","PlayStation","ProtonMail",
  // R
  "Reddit","Revolut","Riyad Bank",
  // S
  "Salesforce","Saudi Post","Shahid","Shopify","Slack",
  "Snapchat","Spotify","STC","STC Pay","stc tv",
  "Steam","Stripe","SurveyMonkey",
  // T
  "Tabby","Talabat","Tamara","TikTok","Trello","Twitch","Twitter",
  // U
  "Uber","Uber Eats","Upwork",
  // V
  "Vimeo","VPN",
  // W
  "WhatsApp","Wise","WordPress",
  // X
  "Xbox","XERO",
  // Y
  "Yahoo","Yahoo Mail","YouTube","YouTube Premium",
  // Z
  "Zain","Zoom",
  // ── Generic / device entries ─────────────────────────────
  "Bank App","Credit Card","Email Account","Home Router",
  "Personal Email","Smart TV","WiFi Router","Work Account",
  "Work Email","Work VPN",
  // ── Gulf / regional extras ────────────────────────────────
  "Almajd TV","Arab National Bank","Astra Tech",
  "Barq","Bayut",
  "Ejada","Emirates NBD",
  "GOSI",
  "Jawwy",
  "Mada","Meem","Mobily Pay",
  "Nana","NHC",
  "Rasan","Rezk",
  "Sahl","Saudi Telecom","SNB Bank","STC Solutions",
  "Taqadam","Tawuniya",
  "Uridu",
  "Wathiq",
  // sentinel
  nullptr
};

const uint16_t SiteDatabase::_count = [](){
  uint16_t n = 0;
  while (SiteDatabase::_db[n]) n++;
  return n;
}();

uint8_t SiteDatabase::search(const char* prefix,
                               const char** results, uint8_t maxN) {
  if (!prefix || prefix[0] == '\0') return 0;
  uint8_t found = 0;
  uint8_t pLen  = strlen(prefix);
  for (uint16_t i = 0; _db[i] && found < maxN; i++) {
    if (strncasecmp(_db[i], prefix, pLen) == 0) {
      results[found++] = _db[i];
    }
  }
  return found;
}

uint16_t SiteDatabase::totalCount() { return _count; }
