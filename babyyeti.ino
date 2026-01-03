#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include <math.h>

/* ================= WIFI ================= */
const char* WIFI_SSID = "xxxxxx";   // 2.4GHz only
const char* WIFI_PASS = "xxxxx#12345";
#define WIFI_TIMEOUT_MS 15000

/* ================= OLED ================= */
#define W 128
#define H 64
#define SDA_PIN 8
#define SCL_PIN 9
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(W, H, &Wire, -1);

/* ================= TOUCH ================= */
#define TOUCH_PIN 7

/* ================= GAME ================= */
#define GROUND_Y 54
#define YETI_SIZE 6

/* ================= TIME ================= */
const char* ntpServer = "pool.ntp.org";
const long IST_OFFSET = 19800;

/* ================= STATES ================= */
enum Mood { IDLE, HAPPY, SLEEPY };
Mood currentMood = IDLE;

enum ScreenMode { SCREEN_YETI, SCREEN_ANALOG, SCREEN_PLUM, SCREEN_GAME };
ScreenMode currentScreen = SCREEN_YETI;

/* ================= TOUCH LOGIC ================= */
bool lastTouch = false;
unsigned long lastTapTime = 0;
int tapCount = 0;

/* ================= ANIMATION ================= */
unsigned long lastBlink = 0;
bool blinkNow = false;

/* ================= GAME STATE ================= */
bool gameRunning = false;
bool gameOver = false;

int yetiY = GROUND_Y - YETI_SIZE;
int velocityY = 0;
int obstacleX = 128;
int score = 0;
unsigned long gameStartHold = 0;

/* ================= UI HELPERS ================= */
void showStatus(const char* msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 28);
  display.println(msg);
  display.display();
}

/* ================= PLUM LAYERS ================= */
void drawPlumLayers() {
  static unsigned long lastSpark = 0;

  display.setTextSize(1);
  display.setCursor(28, 28);
  display.print("Plum Layers");

  if (millis() - lastSpark > 120) {
    lastSpark = millis();
    for (int i = 0; i < 6; i++) {
      display.drawPixel(random(20,108), random(10,54), SSD1306_WHITE);
    }
  }
}

/* ================= EYES ================= */
void drawEyes(Mood mood, bool blink) {
  int y = 28;

  if (blink) {
    display.drawLine(32, y, 48, y, SSD1306_WHITE);
    display.drawLine(80, y, 96, y, SSD1306_WHITE);
    return;
  }

  if (mood == IDLE) {
    display.fillCircle(40, y, 8, SSD1306_WHITE);
    display.fillCircle(88, y, 8, SSD1306_WHITE);
  } else if (mood == HAPPY) {
    display.drawLine(32, y, 48, y, SSD1306_WHITE);
    display.drawLine(34, y+2, 46, y+2, SSD1306_WHITE);
    display.drawLine(80, y, 96, y, SSD1306_WHITE);
    display.drawLine(82, y+2, 94, y+2, SSD1306_WHITE);
  } else {
    display.drawLine(32, y, 48, y, SSD1306_WHITE);
    display.drawLine(80, y, 96, y, SSD1306_WHITE);
  }
}

/* ================= DIGITAL CLOCK ================= */
void drawDigitalClock() {
  struct tm t;
  if (!getLocalTime(&t)) return;

  int h = t.tm_hour;
  bool pm = h >= 12;
  if (h == 0) h = 12;
  else if (h > 12) h -= 12;

  char buf[12];
  snprintf(buf, sizeof(buf), "%02d:%02d %s", h, t.tm_min, pm ? "PM" : "AM");

  int16_t x1, y1;
  uint16_t w, hgt;
  display.getTextBounds(buf, 0, 0, &x1, &y1, &w, &hgt);
  display.setCursor((W - w) / 2, 50);
  display.print(buf);
}

/* ================= ANALOG CLOCK ================= */
void drawAnalogClock() {
  struct tm t;
  if (!getLocalTime(&t)) return;

  int cx = 64, cy = 32, r = 26;
  display.drawCircle(cx, cy, r, SSD1306_WHITE);

  for (int i = 0; i < 60; i++) {
    if (i % 5 == 0) {
      float a = i * 6 * DEG_TO_RAD;
      display.drawLine(cx + cos(a)*(r-1), cy + sin(a)*(r-1),
                       cx + cos(a)*(r-4), cy + sin(a)*(r-4),
                       SSD1306_WHITE);
    }
  }

  float sa = t.tm_sec * 6 * DEG_TO_RAD;
  float ma = (t.tm_min * 6 + t.tm_sec * 0.1) * DEG_TO_RAD;
  float ha = ((t.tm_hour % 12) * 30 + t.tm_min * 0.5) * DEG_TO_RAD;

  display.drawLine(cx, cy, cx + sin(ha)*(r-14), cy - cos(ha)*(r-14), SSD1306_WHITE);
  display.drawLine(cx, cy, cx + sin(ma)*(r-8),  cy - cos(ma)*(r-8),  SSD1306_WHITE);
  display.drawLine(cx, cy, cx + sin(sa)*(r-4),  cy - cos(sa)*(r-4),  SSD1306_WHITE);
  display.fillCircle(cx, cy, 2, SSD1306_WHITE);
}

/* ================= GAME ================= */
void drawGame() {
  display.drawLine(0, GROUND_Y, 128, GROUND_Y, SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(score);

  if (!gameRunning && !gameOver) {
    display.setCursor(30, 24);
    display.print("YETI JUMP");
    display.setCursor(10, 38);
    display.print("Hold 3s to Start");
    return;
  }

  if (gameOver) {
    display.setCursor(30, 24);
    display.print("GAME OVER");
    display.setCursor(14, 38);
    display.print("Hold 3s to Retry");
    return;
  }

  display.fillRect(10, yetiY, YETI_SIZE, YETI_SIZE, SSD1306_WHITE);
  display.fillRect(obstacleX, GROUND_Y - 10, 6, 10, SSD1306_WHITE);

  int speed = min(3 + score / 5, 8);

  velocityY += 1;
  yetiY += velocityY;

  if (yetiY > GROUND_Y - YETI_SIZE) {
    yetiY = GROUND_Y - YETI_SIZE;
    velocityY = 0;
  }

  obstacleX -= speed;
  if (obstacleX < -6) {
    obstacleX = 128 + random(10, 40);
    score++;
  }

  bool hitX = obstacleX < (10 + YETI_SIZE) && obstacleX > 4;
  bool hitY = yetiY + YETI_SIZE > GROUND_Y - 10;

  if (hitX && hitY) {
    gameRunning = false;
    gameOver = true;
  }
}

/* ================= TOUCH ================= */
void handleTouch() {
  bool now = digitalRead(TOUCH_PIN);

  if (now && !lastTouch) {
    unsigned long t = millis();
    tapCount++;

    if (tapCount == 1) lastTapTime = t;
    else if (tapCount == 2 && t - lastTapTime < 400) {
      currentScreen = (ScreenMode)((currentScreen + 1) % 4);
      tapCount = 0;
      lastTouch = now;
      return;
    }
  }

  if (tapCount == 1 && millis() - lastTapTime > 400) {
    if (currentScreen == SCREEN_YETI) {
      currentMood = (Mood)((currentMood + 1) % 3);
    }
    tapCount = 0;
  }

  if (currentScreen == SCREEN_GAME) {
    if (now && !lastTouch && !gameRunning) gameStartHold = millis();

    if (now && !gameRunning && millis() - gameStartHold > 3000) {
      yetiY = GROUND_Y - YETI_SIZE;
      velocityY = 0;
      obstacleX = 128;
      score = 0;
      gameOver = false;
      gameRunning = true;
    }

    if (!now && lastTouch && gameRunning) velocityY = -8;
  }

  lastTouch = now;
}

/* ================= SETUP ================= */
void setup() {
  pinMode(TOUCH_PIN, INPUT);
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.setTextColor(SSD1306_WHITE);

  showStatus("Powering On...");
  delay(800);

  showStatus("Connecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  bool wifiOK = false;

  while (millis() - start < WIFI_TIMEOUT_MS) {
    if (WiFi.status() == WL_CONNECTED) {
      wifiOK = true;
      break;
    }
    display.print(".");
    display.display();
    delay(400);
  }

  if (wifiOK) {
    showStatus("WiFi Connected");
    delay(700);
    configTime(IST_OFFSET, 0, ntpServer);
    showStatus("Time Synced");
    delay(700);
  } else {
    showStatus("WiFi Failed");
    delay(1200);
    showStatus("Offline Mode");
    delay(800);
  }
}

/* ================= LOOP ================= */
void loop() {
  if (millis() - lastBlink > random(4000, 6000)) {
    blinkNow = true;
    lastBlink = millis();
  }

  display.clearDisplay();

  if (currentScreen == SCREEN_YETI) {
    drawEyes(currentMood, blinkNow);
    drawDigitalClock();
  } else if (currentScreen == SCREEN_ANALOG) {
    drawAnalogClock();
  } else if (currentScreen == SCREEN_PLUM) {
    drawPlumLayers();
  } else if (currentScreen == SCREEN_GAME) {
    drawGame();
  }

  display.display();

  if (blinkNow) {
    delay(120);
    blinkNow = false;
  }

  handleTouch();
  delay(60);
}
