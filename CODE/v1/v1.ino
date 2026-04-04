#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include <esp_sntp.h>

#define SDA_PIN         8
#define SCL_PIN         9
#define BOOT_PIN        0
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_ADDRESS  0x3C
#define OLED_ADDRESS  0x3C
#define AP_SSID "LaunchDisplay"
#define AP_PASS "rocketman"
#define AP_IP   "192.168.4.1"
const char* API_URL_BASE =
  "https://ll.thespacedevs.com/2.3.0/launches/upcoming/"
  "?limit=3&format=json"
  "&fields=name,net,net_precision,window_start,window_end,"
           "inhold,holdreason,tbdtime,tbddate,status,mission,pad,rocket";
#define SCROLL_STEP_PX    1
#define SCROLL_DELAY_MS  35
#define SCROLL_PAUSE_MS 1800
struct TZEntry {
  const char* label;   // shown in the portal dropdown
  const char* olson;   // stored in NVS (e.g. "America/New_York")
  const char* posix;   // fed to setenv("TZ", ...) at runtime
};
static const TZEntry TZ_TABLE[] = {
  // ── Americas ──────────────────────────────────────────────────────────────
  {"UTC-12  (Baker Is.)",          "Etc/GMT+12",           "<-12>12"},
  {"UTC-11  (Samoa)",              "Pacific/Pago_Pago",    "SST11"},
  {"UTC-10  (Hawaii)",             "Pacific/Honolulu",     "HST10"},
  {"UTC-9:30(Marquesas)",          "Pacific/Marquesas",    "<-0930>9:30"},
  {"UTC-9   (Alaska)",             "America/Anchorage",    "AKST9AKDT,M3.2.0,M11.1.0"},
  {"UTC-8   (Los Angeles/PT)",     "America/Los_Angeles",  "PST8PDT,M3.2.0,M11.1.0"},
  {"UTC-7   (Denver/MT)",          "America/Denver",       "MST7MDT,M3.2.0,M11.1.0"},
  {"UTC-7   (Phoenix, no DST)",    "America/Phoenix",      "MST7"},
  {"UTC-6   (Chicago/CT)",         "America/Chicago",      "CST6CDT,M3.2.0,M11.1.0"},
  {"UTC-6   (Mexico City)",        "America/Mexico_City",  "CST6CDT,M4.1.0,M10.5.0"},
  {"UTC-5   (New York/ET)",        "America/New_York",     "EST5EDT,M3.2.0,M11.1.0"},
  {"UTC-5   (Bogota, no DST)",     "America/Bogota",       "<-05>5"},
  {"UTC-4   (Halifax/AT)",         "America/Halifax",      "AST4ADT,M3.2.0,M11.1.0"},
  {"UTC-4   (Caracas)",            "America/Caracas",      "<-04>4"},
  {"UTC-3:30(Newfoundland)",       "America/St_Johns",     "NST3:30NDT,M3.2.0,M11.1.0"},
  {"UTC-3   (Sao Paulo)",          "America/Sao_Paulo",    "<-03>3<-02>,M10.3.0,M2.3.0/0"},
  {"UTC-3   (Buenos Aires)",       "America/Argentina/Buenos_Aires", "<-03>3"},
  {"UTC-2   (South Georgia)",      "Atlantic/South_Georgia","<-02>2"},
  {"UTC-1   (Azores)",             "Atlantic/Azores",      "<-01>1<+00>,M3.5.0/0,M10.5.0/1"},
  // ── Europe / Africa ───────────────────────────────────────────────────────
  {"UTC+0   (London/Dublin)",      "Europe/London",        "GMT0BST,M3.5.0/1,M10.5.0"},
  {"UTC+0   (Reykjavik)",          "Atlantic/Reykjavik",   "GMT0"},
  {"UTC+1   (Paris/Berlin/Rome)",  "Europe/Paris",         "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"UTC+1   (Lagos, no DST)",      "Africa/Lagos",         "WAT-1"},
  {"UTC+2   (Helsinki/Kyiv)",      "Europe/Helsinki",      "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"UTC+2   (Cairo)",              "Africa/Cairo",         "EET-2"},
  {"UTC+2   (Johannesburg)",       "Africa/Johannesburg",  "SAST-2"},
  {"UTC+3   (Moscow)",             "Europe/Moscow",        "MSK-3"},
  {"UTC+3   (Nairobi)",            "Africa/Nairobi",       "EAT-3"},
  {"UTC+3   (Istanbul)",           "Europe/Istanbul",      "<+03>-3"},
  {"UTC+3:30(Tehran)",             "Asia/Tehran",          "<+0330>-3:30<+0430>,80/0,264/0"},
  {"UTC+4   (Dubai)",              "Asia/Dubai",           "<+04>-4"},
  {"UTC+4:30(Kabul)",              "Asia/Kabul",           "<+0430>-4:30"},
  {"UTC+5   (Karachi)",            "Asia/Karachi",         "PKT-5"},
  {"UTC+5:30(India)",              "Asia/Kolkata",         "IST-5:30"},
  {"UTC+5:45(Kathmandu)",          "Asia/Kathmandu",       "<+0545>-5:45"},
  {"UTC+6   (Dhaka/Almaty)",       "Asia/Dhaka",           "<+06>-6"},
  {"UTC+6:30(Yangon)",             "Asia/Yangon",          "<+0630>-6:30"},
  {"UTC+7   (Bangkok/Jakarta)",    "Asia/Bangkok",         "<+07>-7"},
  {"UTC+8   (Beijing/Singapore)",  "Asia/Singapore",       "<+08>-8"},
  {"UTC+8   (Perth, no DST)",      "Australia/Perth",      "AWST-8"},
  {"UTC+8:45(Eucla)",              "Australia/Eucla",      "<+0845>-8:45"},
  {"UTC+9   (Tokyo/Seoul)",        "Asia/Tokyo",           "JST-9"},
  {"UTC+9:30(Adelaide)",           "Australia/Adelaide",   "ACST-9:30ACDT,M10.1.0,M4.1.0/3"},
  {"UTC+9:30(Darwin, no DST)",     "Australia/Darwin",     "ACST-9:30"},
  {"UTC+10  (Sydney/Melbourne)",   "Australia/Sydney",     "AEST-10AEDT,M10.1.0,M4.1.0/3"},
  {"UTC+10  (Brisbane, no DST)",   "Australia/Brisbane",   "AEST-10"},
  {"UTC+10:30(Lord Howe Is.)",     "Australia/Lord_Howe",  "<+1030>-10:30<+11>-11,M10.1.0,M4.1.0"},
  {"UTC+11  (Noumea)",             "Pacific/Noumea",       "<+11>-11"},
  {"UTC+12  (Auckland)",           "Pacific/Auckland",     "NZST-12NZDT,M9.5.0,M4.1.0/3"},
  {"UTC+12  (Fiji)",               "Pacific/Fiji",         "<+12>-12"},
  {"UTC+12:45(Chatham Is.)",       "Pacific/Chatham",      "<+1245>-12:45<+1345>,M9.5.0/2:45,M4.1.0/3:45"},
  {"UTC+13  (Tonga)",              "Pacific/Tongatapu",    "<+13>-13"},
  {"UTC+14  (Kiritimati)",         "Pacific/Kiritimati",   "<+14>-14"},
};
static const uint8_t TZ_COUNT = sizeof(TZ_TABLE) / sizeof(TZ_TABLE[0]);

// Default: UTC
#define TZ_DEFAULT_IDX  19   // "UTC+0 (London/Dublin)" — first UTC+0 entry
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Preferences       prefs;
WebServer         server(80);

String g_ssid;
String g_pass;
String g_tzOlson;          // e.g. "America/New_York"
String g_tzPosix;          // e.g. "EST5EDT,M3.2.0,M11.1.0"

volatile bool g_ntpSynced = false;

// ── Launch data ───────────────────────────────────────────────────────────────
struct LaunchData {
  String missionName;
  String rocketName;
  String padName;
  String status;           // "Go", "TBD", "Hold", etc.
  String statusFull;       // abbrev already, but keep for reference

  time_t t0Epoch    = 0;   // best T-0: window_start if precise, else net
  bool   t0Precise  = false; // true when window_start == window_end & !tbd
  bool   inHold     = false;
  bool   tbdTime    = false;
  bool   tbdDate    = false;
  String holdReason;

  String dateLocal;        // formatted local date string for display
  String timeLocal;        // formatted local time + tz abbreviation

  bool   valid = false;
};
LaunchData    g_launch;
unsigned long g_lastFetch     = 0;
unsigned long g_refreshPeriod = 5UL * 60UL * 1000UL;   // adaptive
String trunc(const String& s, uint8_t n) {
  return s.length() <= n ? s : s.substring(0, n-1) + '.';
}

// Parse ISO-8601 "2026-04-01T14:30:00Z" → UTC epoch (no libc mktime TZ risk)
time_t isoToEpoch(const String& iso) {
  if (iso.length() < 19) return 0;
  int Y = iso.substring(0,  4).toInt();
  int M = iso.substring(5,  7).toInt();
  int D = iso.substring(8, 10).toInt();
  int h = iso.substring(11, 13).toInt();
  int m = iso.substring(14, 16).toInt();
  int s = iso.substring(17, 19).toInt();
  const int dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  long days = 0;
  for (int y = 1970; y < Y; y++)
    days += (y%4==0 && (y%100!=0||y%400==0)) ? 366 : 365;
  for (int mo = 1; mo < M; mo++) {
    days += dim[mo-1];
    if (mo==2 && Y%4==0 && (Y%100!=0||Y%400==0)) days++;
  }
  days += D-1;
  return (time_t)(days*86400L + h*3600L + m*60L + s);
}
void epochToLocalStrings(time_t epoch, String& outDate, String& outTime) {
  struct tm t;
  localtime_r(&epoch, &t);   // uses TZ env var → DST-aware
  char buf[24];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
           t.tm_year+1900, t.tm_mon+1, t.tm_mday);
  outDate = buf;
  // strftime %Z gives the local tz abbreviation (e.g. "EDT", "BST", "CEST")
  char tz[8] = {};
  strftime(tz, sizeof(tz), "%Z", &t);
  snprintf(buf, sizeof(buf), "%02d:%02d %s", t.tm_hour, t.tm_min, tz);
  outTime = buf;
}

// Countdown string: "T-02d 14:30:05" or "L+00d 00:05:12"
String formatCountdown(long diffSec) {
  bool before = (diffSec >= 0);
  long a = abs(diffSec);
  char buf[20];
  snprintf(buf, sizeof(buf), "%s%02ldd %02ld:%02ld:%02ld",
           before ? "T-" : "L+",
           a/86400L, (a%86400L)/3600L, (a%3600L)/60L, a%60L);
  return String(buf);
}

// Live countdown; respects hold/TBD states
String liveCountdown() {
  if (g_launch.tbdDate) return F("T-? (date TBD)");
  if (g_launch.tbdTime) return F("T-? (time TBD)");
  if (g_launch.inHold)  return F("ON HOLD");
  time_t now = time(nullptr);
  if (now < 100000L)    return F("NTP syncing...");
  return formatCountdown((long)g_launch.t0Epoch - (long)now);
}

// Adaptive refresh: faster near T-0
void updateRefreshPeriod() {
  time_t now  = time(nullptr);
  long   diff = (long)g_launch.t0Epoch - (long)now;
  if (diff < 0)          g_refreshPeriod = 5UL*60*1000;  // post-launch: 5 min
  else if (diff <  600)  g_refreshPeriod =       30*1000; // <10 min : 30 s
  else if (diff < 3600)  g_refreshPeriod =       60*1000; // <1 hr   : 60 s
  else if (diff < 86400) g_refreshPeriod =  2UL*60*1000;  // <1 day  : 2 min
  else                   g_refreshPeriod =  5UL*60*1000;  // default : 5 min
}

// ─────────────────────────────────────────────────────────────────────────────
//  TZ LOOKUP
// ─────────────────────────────────────────────────────────────────────────────

// Return index into TZ_TABLE for saved IANA name; default if not found
uint8_t findTZIndex(const String& olson) {
  for (uint8_t i = 0; i < TZ_COUNT; i++)
    if (olson == TZ_TABLE[i].olson) return i;
  return TZ_DEFAULT_IDX;
}

void applyTimezone(const String& posixStr) {
  setenv("TZ", posixStr.c_str(), 1);
  tzset();
}

// ─────────────────────────────────────────────────────────────────────────────
//  NVS SETTINGS
// ─────────────────────────────────────────────────────────────────────────────

void loadSettings() {
  prefs.begin("launch", true);
  g_ssid     = prefs.getString("ssid",    "");
  g_pass     = prefs.getString("pass",    "");
  g_tzOlson  = prefs.getString("tzOlson", TZ_TABLE[TZ_DEFAULT_IDX].olson);
  prefs.end();
  uint8_t idx = findTZIndex(g_tzOlson);
  g_tzPosix   = TZ_TABLE[idx].posix;
  applyTimezone(g_tzPosix);
}

void saveSettings(const String& ssid, const String& pass, const String& olson) {
  prefs.begin("launch", false);
  prefs.putString("ssid",    ssid);
  prefs.putString("pass",    pass);
  prefs.putString("tzOlson", olson);
  prefs.end();
}

// ─────────────────────────────────────────────────────────────────────────────
//  OLED HELPERS
// ─────────────────────────────────────────────────────────────────────────────

void oledMsg(const char* l1, const char* l2=nullptr,
             const char* l3=nullptr, const char* l4=nullptr) {
  display.clearDisplay();
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  int y=2;
  auto p=[&](const char*s){ if(s){display.setCursor(0,y);display.println(s);}y+=12;};
  p(l1);p(l2);p(l3);p(l4);
  display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
//  CONFIG PORTAL
// ─────────────────────────────────────────────────────────────────────────────

String buildTZOptions(const String& curOlson) {
  String s;
  for (uint8_t i = 0; i < TZ_COUNT; i++) {
    bool sel = (curOlson == TZ_TABLE[i].olson);
    s += "<option value=\"";
    s += TZ_TABLE[i].olson;
    s += "\"";
    if (sel) s += " selected";
    s += ">";
    s += TZ_TABLE[i].label;
    s += "</option>\n";
  }
  return s;
}

void handleRoot() {
  String html;
  html.reserve(4096);
  html += F("<!DOCTYPE html><html><head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Launch Display Setup</title>"
    "<style>"
    "body{font-family:sans-serif;background:#0d0d1a;color:#dde;margin:0;"
         "display:flex;align-items:center;justify-content:center;min-height:100vh}"
    ".c{background:#1a1a2e;border:1px solid #334;border-radius:12px;"
        "padding:26px 30px;max-width:380px;width:100%}"
    "h2{margin:0 0 4px;color:#7ec8ff;font-size:1.2rem}"
    "p{margin:0 0 18px;color:#889;font-size:.82rem}"
    "label{display:block;font-size:.78rem;color:#aab;margin-bottom:3px}"
    "input,select{width:100%;box-sizing:border-box;padding:8px 10px;"
      "border-radius:6px;border:1px solid #445;background:#111;color:#dde;"
      "font-size:.88rem;margin-bottom:14px}"
    "button{width:100%;padding:10px;background:#2563eb;color:#fff;border:none;"
      "border-radius:7px;font-size:.95rem;cursor:pointer;font-weight:600}"
    "button:hover{background:#1d4ed8}"
    ".note{font-size:.75rem;color:#668;margin-top:-10px;margin-bottom:12px}"
    ".ico{text-align:center;font-size:2rem;margin-bottom:10px}"
    "</style></head><body><div class='c'>"
    "<div class='ico'>&#x1F680;</div>"
    "<h2>Launch Display  v3</h2>"
    "<p>Set your WiFi and choose your local timezone (DST handled automatically).</p>"
    "<form method='POST' action='/save'>"
    "<label>WiFi SSID</label>"
    "<input name='ssid' required value='");
  html += g_ssid;
  html += F("'>"
    "<label>WiFi Password</label>"
    "<input name='pass' type='password' value='");
  html += g_pass;
  html += F("'>"
    "<label>Timezone</label>"
    "<select name='tz'>");
  html += buildTZOptions(g_tzOlson);
  html += F("</select>"
    "<p class='note'>DST transitions are automatic &mdash; no manual offset needed.</p>"
    "<button type='submit'>Save &amp; Connect &#x2192;</button>"
    "</form></div></body></html>");
  server.send(200, "text/html", html);
}

void handleSave() {
  String ssid  = server.arg("ssid");
  String pass  = server.arg("pass");
  String olson = server.arg("tz");

  // Validate olson is in our table
  uint8_t idx = findTZIndex(olson);
  if (idx >= TZ_COUNT) olson = TZ_TABLE[TZ_DEFAULT_IDX].olson;

  saveSettings(ssid, pass, olson);
  server.send(200, "text/html",
    "<html><body style='font-family:sans-serif;background:#0d0d1a;color:#dde;"
    "text-align:center;padding-top:60px'>"
    "<h2 style='color:#7ec8ff'>&#x2705; Saved!</h2>"
    "<p>Rebooting&hellip;</p></body></html>");
  delay(1500);
  ESP.restart();
}

void runConfigPortal() {
  oledMsg(
  "Config mode",
  (String("WiFi: ") + AP_SSID).c_str(),
  "Pass: rocketman",
  (String("http://") + AP_IP).c_str()
);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(
    IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  WiFi.softAP(AP_SSID, AP_PASS);
  server.on("/",     HTTP_GET,  handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound([](){
    server.sendHeader("Location", String("http://") + AP_IP, true);
    server.send(302,"text/plain","");
  });
  server.begin();
  while (true) { server.handleClient(); yield(); }
}

// ─────────────────────────────────────────────────────────────────────────────
//  WIFI + NTP
// ─────────────────────────────────────────────────────────────────────────────

void ntpCb(struct timeval*) { g_ntpSynced = true; }

void connectWiFi() {
  oledMsg("Connecting WiFi...", g_ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(g_ssid.c_str(), g_pass.c_str());
  uint8_t tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (++tries > 60) { oledMsg("WiFi failed!","Starting portal..."); delay(1500); runConfigPortal(); }
  }
  // NTP always in UTC; TZ env var handles local display conversion
  sntp_set_time_sync_notification_cb(ntpCb);
  configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
  uint32_t t0 = millis();
  while (!g_ntpSynced && millis()-t0 < 10000) delay(200);
  oledMsg("WiFi OK", WiFi.localIP().toString().c_str(),
          g_ntpSynced ? "NTP synced" : "NTP pending...");
  delay(900);
}

// ─────────────────────────────────────────────────────────────────────────────
//  LAUNCH API FETCH
// ─────────────────────────────────────────────────────────────────────────────
String getNowISO8601() {
  time_t now = time(nullptr);
  if (now < 100000) return ""; // NTP not ready

  struct tm t;
  gmtime_r(&now, &t);

  char buf[25];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &t);
  return String(buf);
}
bool fetchLaunch() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  oledMsg("Fetching launch...");

  HTTPClient http;
  String nowISO = getNowISO8601();
String url = String(API_URL_BASE);

if (nowISO.length() > 0) {
  url += "&net__gte=" + nowISO;
}

http.begin(url);
  http.addHeader("User-Agent", "ESP32-LaunchDisplay/3.0");
  http.setTimeout(15000);

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("HTTP %d\n", code); http.end(); return false;
  }

  // Filter keeps only fields we use — saves heap
  JsonDocument filter;
  auto r0 = filter["results"][0];
  r0["name"]                             = true;
  r0["net"]                              = true;
  r0["net_precision"]["name"]            = true;
  r0["window_start"]                     = true;
  r0["window_end"]                       = true;
  r0["inhold"]                           = true;
  r0["holdreason"]                       = true;
  r0["tbdtime"]                          = true;
  r0["tbddate"]                          = true;
  r0["status"]["abbrev"]                 = true;
  r0["mission"]["name"]                  = true;
  r0["rocket"]["configuration"]["name"]  = true;
  r0["pad"]["name"]                      = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(
    doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) { Serial.printf("JSON: %s\n", err.c_str()); return false; }

JsonArray results = doc["results"];
if (results.isNull() || results.size() == 0) {
  Serial.println(F("No results"));
  return false;
}

JsonObject lj;
time_t now = time(nullptr);

// Find first valid future launch
for (JsonObject obj : results) {
  String netStr = obj["net"] | "";
  time_t t = isoToEpoch(netStr);

  if (t > now - 60) { // allow slight clock skew
    lj = obj;
    break;
  }
}

// Fallback: just take first if none matched
if (lj.isNull()) {
  Serial.println(F("No future launch found, using first result"));
  lj = results[0];
}

  // ── Mission / rocket / pad ──────────────────────────────────────────────
  const char* mn = lj["mission"]["name"] | (const char*)nullptr;
  if (!mn || !*mn) mn = lj["name"] | "Unknown";
  g_launch.missionName = mn;
  g_launch.rocketName  = lj["rocket"]["configuration"]["name"] | "Unknown";
  g_launch.padName     = lj["pad"]["name"] | "Unknown";
  g_launch.status      = lj["status"]["abbrev"] | "?";

  // ── Hold / TBD flags ────────────────────────────────────────────────────
  g_launch.inHold     = lj["inhold"]  | false;
  g_launch.tbdTime    = lj["tbdtime"] | false;
  g_launch.tbdDate    = lj["tbddate"] | false;
  g_launch.holdReason = lj["holdreason"] | String("");

  // ── Determine best T-0 epoch ────────────────────────────────────────────
  // "Exact" T-0 = window_start == window_end AND net_precision is
  // "Second" or "Minute" AND not tbdtime/tbddate.
  // Otherwise fall back to NET.
  String netStr    = lj["net"]          | String("");
  String winStart  = lj["window_start"] | String("");
  String winEnd    = lj["window_end"]   | String("");
  const char* prec = lj["net_precision"]["name"] | "Day";

  bool preciseNET = (strcmp(prec,"Second")==0 || strcmp(prec,"Minute")==0
                                               || strcmp(prec,"Hour")==0);
  bool windowPinned = (winStart == winEnd) && winStart.length() >= 19;

  if (!g_launch.tbdTime && !g_launch.tbdDate && windowPinned && preciseNET) {
    // window_start IS T-0
    g_launch.t0Epoch   = isoToEpoch(winStart);
    g_launch.t0Precise = true;
  } else if (!g_launch.tbdDate && preciseNET) {
    // NET is the best we have, but has at least hour precision
    g_launch.t0Epoch   = isoToEpoch(netStr);
    g_launch.t0Precise = false;
  } else {
    // Only date-level or no info — use NET anyway, mark imprecise
    g_launch.t0Epoch   = isoToEpoch(netStr);
    g_launch.t0Precise = false;
  }

  // ── Format local display strings ────────────────────────────────────────
  epochToLocalStrings(g_launch.t0Epoch, g_launch.dateLocal, g_launch.timeLocal);

  g_launch.valid = true;
  updateRefreshPeriod();

  Serial.printf("Launch: %s | T-0 epoch: %ld | precise: %s | hold: %s | tbd: %s/%s\n",
    g_launch.missionName.c_str(), (long)g_launch.t0Epoch,
    g_launch.t0Precise?"Y":"N",
    g_launch.inHold?"Y":"N",
    g_launch.tbdDate?"date":"–", g_launch.tbdTime?"time":"–");
  return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  DISPLAY — shared header bar
// ─────────────────────────────────────────────────────────────────────────────

void drawHeader() {
  display.fillRect(0, 0, SCREEN_WIDTH, 9, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(2, 1);
  display.print(F("NEXT LAUNCH"));
  // Status pill — show "HOLD" in priority if on hold
  display.setCursor(75, 1);
  if (g_launch.inHold)        display.print(F("** HOLD **"));
  else if (g_launch.tbdDate)  display.print(F("TBD DATE"));
  else if (g_launch.tbdTime)  display.print(F("TBD TIME"));
  else {
    display.print(F("ST:"));
    display.print(g_launch.status);
  }
  display.setTextColor(SSD1306_WHITE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  DISPLAY — Screen A  (mission info + compact countdown)
// ─────────────────────────────────────────────────────────────────────────────
/*
  ┌────────────────────────────────┐
  │ NEXT LAUNCH       ST: Go      │  y=0  (inverted bar)
  │ <mission — scrolls>           │  y=10
  │ » Falcon 9 Block 5            │  y=20
  │ <pad — scrolls>               │  y=30
  │────────────────────────────────│  y=40
  │ 2026-04-01  14:30 EDT         │  y=42
  │ T-02d 14:30:05  [~]           │  y=53  live, updates each scroll step
  └────────────────────────────────┘
*/

void drawFrameA() {
  display.clearDisplay();
  drawHeader();
  display.setCursor(0, 20);
  display.print(F("\xBB "));
  display.print(trunc(g_launch.rocketName, 18));
  display.drawFastHLine(0, 40, SCREEN_WIDTH, SSD1306_WHITE);
  display.setCursor(0, 42);
  display.print(trunc(g_launch.dateLocal + "  " + g_launch.timeLocal, 21));
  display.setCursor(0, 53);
  String cd = liveCountdown();
  if (!g_launch.t0Precise && !g_launch.inHold &&
      !g_launch.tbdTime && !g_launch.tbdDate)
    cd += F(" ~");
  display.print(cd);
}

void showScreenA() {
  // Scroll mission at y=10
  int16_t mW = g_launch.missionName.length() * 6;
  if (mW > SCREEN_WIDTH) {
    for (int16_t x=0; x>=-(mW-SCREEN_WIDTH); x-=SCROLL_STEP_PX) {
      drawFrameA();
      display.setCursor(x,10); display.print(g_launch.missionName);
      display.setCursor(0,30); display.print(trunc(g_launch.padName,21));
      display.display(); delay(SCROLL_DELAY_MS);
    }
    delay(SCROLL_PAUSE_MS/2);
  } else {
    drawFrameA();
    display.setCursor(0,10); display.print(g_launch.missionName);
    display.setCursor(0,30); display.print(trunc(g_launch.padName,21));
    display.display(); delay(SCROLL_PAUSE_MS*2);
  }
  // Scroll pad at y=30
  int16_t pW = g_launch.padName.length() * 6;
  if (pW > SCREEN_WIDTH) {
    for (int16_t x=0; x>=-(pW-SCREEN_WIDTH); x-=SCROLL_STEP_PX) {
      drawFrameA();
      display.setCursor(0,10); display.print(trunc(g_launch.missionName,21));
      display.setCursor(x,30); display.print(g_launch.padName);
      display.display(); delay(SCROLL_DELAY_MS);
    }
    delay(SCROLL_PAUSE_MS/2);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  DISPLAY — Screen B  (large countdown)
// ─────────────────────────────────────────────────────────────────────────────
/*
  ┌────────────────────────────────┐
  │ NEXT LAUNCH       ST: Go      │  y=0
  │ <mission — scrolls>           │  y=10
  │────────────────────────────────│  y=20
  │          T-02d                │  y=23  size 2
  │       14 : 30 : 05            │  y=43  size 2
  │ ~approx                       │  y=57  size 1 (if imprecise)
  └────────────────────────────────┘

  Hold / TBD variant:
  │────────────────────────────────│  y=20
  │         ON HOLD                │  y=28  size 2 (centred)
  │   <hold reason — scrolls>     │  y=50  size 1
  └────────────────────────────────┘
*/

void showScreenB() {
  int16_t mW   = g_launch.missionName.length() * 6;
  int16_t scrX = 0;

  unsigned long start = millis();
  while (millis() - start < 6000) {
    display.clearDisplay();
    drawHeader();

    // Mission (auto-scroll)
    if (mW > SCREEN_WIDTH) {
      display.setCursor(scrX, 10); display.print(g_launch.missionName);
      scrX -= SCROLL_STEP_PX;
      if (scrX < -(mW - SCREEN_WIDTH)) scrX = 0;
    } else {
      display.setCursor(0, 10); display.print(g_launch.missionName);
    }

    display.drawFastHLine(0, 20, SCREEN_WIDTH, SSD1306_WHITE);
    display.setTextSize(2);

    if (g_launch.inHold) {
      // ── HOLD screen ───────────────────────────────────────────────────
      const char* msg = "ON HOLD";
      int16_t w = strlen(msg) * 12;
      display.setCursor((SCREEN_WIDTH-w)/2, 26);
      display.print(msg);
      display.setTextSize(1);
      // Scroll hold reason if present
      if (g_launch.holdReason.length() > 0) {
        display.setCursor(0, 50);
        display.print(trunc(g_launch.holdReason, 21));
      }
    } else if (g_launch.tbdDate || g_launch.tbdTime) {
      // ── TBD screen ────────────────────────────────────────────────────
      const char* msg = g_launch.tbdDate ? "DATE TBD" : "TIME TBD";
      int16_t w = strlen(msg) * 12;
      display.setCursor((SCREEN_WIDTH-w)/2, 26);
      display.print(msg);
      display.setTextSize(1);
      display.setCursor(0, 50);
      display.print(g_launch.dateLocal);
    } else {
      // ── Normal countdown ──────────────────────────────────────────────
      time_t now  = time(nullptr);
      long   diff = (now > 100000L) ? (long)g_launch.t0Epoch - (long)now : 0;
      bool   before = (diff >= 0);
      long   a  = abs(diff);
      long   dd = a / 86400L;
      long   hh = (a % 86400L) / 3600L;
      long   mm = (a % 3600L)  / 60L;
      long   ss = a % 60L;

      char top[10], bot[10];
      snprintf(top, sizeof(top), "%s%02ldd", before?"T-":"L+", dd);
      snprintf(bot, sizeof(bot), "%02ld:%02ld:%02ld", hh, mm, ss);

      int16_t tw = strlen(top)*12;
      display.setCursor((SCREEN_WIDTH-tw)/2, 23);
      display.print(top);

      int16_t bw = strlen(bot)*12;
      display.setCursor((SCREEN_WIDTH-bw)/2, 43);
      display.print(bot);

      display.setTextSize(1);
      if (!g_launch.t0Precise) {
        display.setCursor(0, 57);
        display.print(F("~window not pinned"));
      }
    }

    display.setTextSize(1);
    display.display();
    delay(250);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
//  SETUP & LOOP
// ─────────────────────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);

  pinMode(BOOT_PIN, INPUT_PULLUP);
  delay(100);
  bool forcePortal = (digitalRead(BOOT_PIN) == LOW);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("SSD1306 init failed"));
    while (true) delay(1000);
  }
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);

  // Splash
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 16); display.println(F(" LAUNCH  DISPLAY  v3"));
  display.setCursor(36, 30); display.println(F("Booting..."));
  display.display(); delay(1200);

  loadSettings();   // also calls applyTimezone() → tzset()

  if (forcePortal || g_ssid.isEmpty()) runConfigPortal();  // never returns

  connectWiFi();

  if (!fetchLaunch()) {
    oledMsg("Fetch failed!", "Retry in 60s...");
    delay(60000);
  }
  g_lastFetch = millis();
}

void loop() {
  // Adaptive refresh
  updateRefreshPeriod();
  if (millis() - g_lastFetch >= g_refreshPeriod) {
    fetchLaunch();
    g_lastFetch = millis();
  }

  if (!g_launch.valid) {
    oledMsg("No launch data.", "Check WiFi/API.");
    delay(3000);
    return;
  }

  showScreenA();   // info + compact countdown
  showScreenB();   // large T-/L+ or hold/TBD display
}
