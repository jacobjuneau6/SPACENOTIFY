#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// -------- WIFI --------
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// -------- ST7735 --------
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// -------- OLED --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// -------- TOUCH --------
#define TOUCH_PIN 15

// -------- NTP --------
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 1000);

// -------- API --------
const char* apiUrl = "https://fdo.rocketlaunch.live/json/launches/next/5";

// -------- DATA --------
struct Launch {
  String name;
  String provider;
  String vehicle;
  String location;
  time_t epoch;
};

Launch launches[5];
int currentIndex = 0;
bool detailView = false;

// -------- TOUCH STATE --------
bool lastTouch = LOW;
unsigned long touchStart = 0;

// -------- AUTO SWITCH --------
bool liftoffHandled = false;
unsigned long liftoffTime = 0;

// -------- REFRESH CONTROL --------
unsigned long lastRefresh = 0;
const unsigned long NORMAL_REFRESH = 600000; // 10 min

// -------- TIME PARSE --------
time_t parseISOTime(String iso) {
  struct tm t;
  sscanf(iso.c_str(), "%d-%d-%dT%d:%d:%d",
         &t.tm_year, &t.tm_mon, &t.tm_mday,
         &t.tm_hour, &t.tm_min, &t.tm_sec);

  t.tm_year -= 1900;
  t.tm_mon -= 1;
  return mktime(&t);
}

// -------- FETCH --------
void fetchLaunches() {
  HTTPClient http;
  http.begin(apiUrl);

  if (http.GET() == 200) {
    DynamicJsonDocument doc(16384);
    deserializeJson(doc, http.getString());

    JsonArray result = doc["result"];

    for (int i = 0; i < 5; i++) {
      launches[i].name = result[i]["name"].as<String>();
      launches[i].provider = result[i]["provider"]["name"].as<String>();
      launches[i].vehicle = result[i]["vehicle"]["name"].as<String>();
      launches[i].location = result[i]["pad"]["location"]["name"].as<String>();

      String iso = result[i]["date_utc"].as<String>();
      launches[i].epoch = parseISOTime(iso);
    }
  }

  http.end();
  lastRefresh = millis();
}

// -------- COUNTDOWN --------
String getCountdown(time_t target) {
  long diff = target - timeClient.getEpochTime();

  if (diff <= 0) return "LIFTOFF";

  int h = diff / 3600;
  int m = (diff % 3600) / 60;
  int s = diff % 60;

  char buf[20];
  sprintf(buf, "T-%02d:%02d:%02d", h, m, s);
  return String(buf);
}

// -------- DRAW LIST --------
void drawList() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);

  for (int i = 0; i < 5; i++) {
    int y = i * 25;

    if (i == currentIndex) {
      tft.fillRect(0, y, 160, 25, ST77XX_BLUE);
      tft.setTextColor(ST77XX_BLACK);
    } else {
      tft.setTextColor(ST77XX_WHITE);
    }

    tft.setCursor(2, y + 2);
    tft.print(launches[i].name.substring(0, 18));

    tft.setCursor(2, y + 14);
    tft.print(getCountdown(launches[i].epoch));
  }
}

// -------- DETAIL --------
void drawDetail() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);

  Launch l = launches[currentIndex];

  tft.setCursor(0, 0);
  tft.println(l.name);
  tft.println(l.vehicle);
  tft.println(l.provider);
  tft.println(l.location);

  tft.println();
  tft.setTextSize(2);
  tft.println(getCountdown(l.epoch));
  tft.setTextSize(1);
}

// -------- OLED --------
void drawOLED() {
  oled.clearDisplay();

  Launch l = launches[0];

  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("NEXT:");

  oled.println(l.name.substring(0, 16));

  oled.setTextSize(2);
  oled.setCursor(0, 30);
  oled.println(getCountdown(l.epoch));

  oled.display();
}

// -------- TOUCH --------
void handleTouch() {
  bool state = digitalRead(TOUCH_PIN);

  if (state == HIGH && lastTouch == LOW)
    touchStart = millis();

  if (state == LOW && lastTouch == HIGH) {
    unsigned long d = millis() - touchStart;

    if (d > 800) {
      detailView = !detailView;
    } else {
      if (!detailView)
        currentIndex = (currentIndex + 1) % 5;
    }
  }

  lastTouch = state;
}

// -------- AUTO SWITCH --------
void handleAutoSwitch() {
  long diff = launches[0].epoch - timeClient.getEpochTime();

  if (diff <= 0 && !liftoffHandled) {
    liftoffHandled = true;
    liftoffTime = millis();
  }

  if (liftoffHandled && millis() - liftoffTime > 5000) {
    fetchLaunches(); // refresh instead of shifting

    currentIndex = 0;
    detailView = false;

    liftoffHandled = false;
  }
}

// -------- SMART REFRESH --------
void handleSmartRefresh() {
  long diff = launches[0].epoch - timeClient.getEpochTime();

  // Normal refresh every 10 min
  if (millis() - lastRefresh > NORMAL_REFRESH) {
    fetchLaunches();
  }

  // If launch is soon (<15 min), refresh more often
  if (diff > 0 && diff < 900) {
    if (millis() - lastRefresh > 60000) { // every 1 min
      fetchLaunches();
    }
  }
}

// -------- SETUP --------
void setup() {
  Serial.begin(115200);

  pinMode(TOUCH_PIN, INPUT);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  timeClient.begin();
  timeClient.update();

  fetchLaunches();
}

// -------- LOOP --------
void loop() {
  handleTouch();
  timeClient.update();

  handleAutoSwitch();
  handleSmartRefresh();

  static unsigned long lastDraw = 0;

  if (millis() - lastDraw > 1000) {
    if (detailView) drawDetail();
    else drawList();

    drawOLED();

    lastDraw = millis();
  }
}