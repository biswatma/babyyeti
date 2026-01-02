#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include <math.h>

/* ================= WIFI ================= */
const char* WIFI_SSID = "Pixel 2.4";
const char* WIFI_PASS = "biswatma#12345";

/* ================= OLED ================= */
#define W 128
#define H 64
#define SDA_PIN 8
#define SCL_PIN 9
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(W, H, &Wire, -1);

/* ================= TOUCH ================= */
#define TOUCH_PIN 7

/* ================= GAME CONST ================= */
#define GROUND_Y 54
#define YETI_SIZE 6

/* ================= TIME ================= */
const char* ntpServer = "pool.ntp.org";
const long IST_OFFSET = 19800;

/* ================= STATES ================= */
enum Mood { IDLE, HAPPY, SLEEPY };
Mood currentMood = IDLE;

enum ScreenMode {
  SCREEN_YETI,
  SCREEN_ANALOG,
  SCREEN_PLUM,
  SCREEN_GAME_JUMP,
  SCREEN_GAME_FLAP
};
ScreenMode currentScreen = SCREEN_YETI;

/* ================= TOUCH ================= */
bool lastTouch = false;
unsigned long lastTapTime = 0;
int tapCount = 0;

/* ================= ANIMATION ================= */
unsigned long lastBlink = 0;
bool blinkNow = false;

/* ================= GAME COMMON ================= */
bool gameRunning = false;
bool gameOver = false;
unsigned long gameStartHold = 0;

/* ================= YETI JUMP ================= */
int jumpY = GROUND_Y - YETI_SIZE;
int jumpVel = 0;
int obstacleX = 128;
int jumpScore = 0;

/* ================= YETI FLAP ================= */
float flapY = 30;
float flapVel = 0;
int pipeX = 128;
int pipeGapY = 18;
int flapScore = 0;

/* ========================================================= */

void resetGames() {
  gameRunning = false;
  gameOver = false;
  jumpScore = flapScore = 0;
  obstacleX = pipeX = 128;
  jumpY = GROUND_Y - YETI_SIZE;
  flapY = 30;
  jumpVel = flapVel = 0;
}

void drawEyes(Mood mood, bool blink) {
  int y = 28;
  if (blink) {
    display.drawLine(32,y,48,y,SSD1306_WHITE);
    display.drawLine(80,y,96,y,SSD1306_WHITE);
    return;
  }
  display.fillCircle(40,y,8,SSD1306_WHITE);
  display.fillCircle(88,y,8,SSD1306_WHITE);
}

void drawDigitalClock() {
  struct tm t;
  if (!getLocalTime(&t)) return;
  int h=t.tm_hour; bool pm=h>=12;
  if(h==0)h=12; else if(h>12)h-=12;
  char buf[12];
  snprintf(buf,sizeof(buf),"%02d:%02d %s",h,t.tm_min,pm?"PM":"AM");
  int16_t x1,y1; uint16_t w,hg;
  display.getTextBounds(buf,0,0,&x1,&y1,&w,&hg);
  display.setCursor((W-w)/2,50);
  display.print(buf);
}

void drawPlumLayers() {
  display.setCursor(28,28);
  display.print("Plum Layers");
  for(int i=0;i<4;i++)
    display.drawPixel(random(20,108),random(10,54),SSD1306_WHITE);
}

/* ================= YETI JUMP ================= */
void drawYetiJump() {
  display.drawLine(0,GROUND_Y,128,GROUND_Y,SSD1306_WHITE);
  display.setCursor(0,0); display.print("Score: "); display.print(jumpScore);

  if (!gameRunning && !gameOver) {
    display.setCursor(20,28); display.print("YETI JUMP");
    display.setCursor(8,40); display.print("Hold 3s to Start");
    return;
  }
  if (gameOver) {
    display.setCursor(30,28); display.print("GAME OVER");
    display.setCursor(8,40); display.print("Hold 3s to Retry");
    return;
  }

  display.fillRect(10,jumpY,YETI_SIZE,YETI_SIZE,SSD1306_WHITE);
  display.fillRect(obstacleX,GROUND_Y-10,6,10,SSD1306_WHITE);

  jumpVel += 1;
  jumpY += jumpVel;
  if(jumpY > GROUND_Y-YETI_SIZE){ jumpY = GROUND_Y-YETI_SIZE; jumpVel=0; }

  int speed = min(7, 3 + jumpScore/6);
  obstacleX -= speed;
  if(obstacleX < -6){ obstacleX=128; jumpScore++; }

  if(obstacleX < 16 && obstacleX > 4 && jumpY+YETI_SIZE > GROUND_Y-10){
    gameOver=true; gameRunning=false;
  }
}

/* ================= YETI FLAP ================= */
void drawYetiFlap() {
  display.setCursor(0,0); display.print("Score: "); display.print(flapScore);

  if (!gameRunning && !gameOver) {
    display.setCursor(20,28); display.print("YETI FLAP");
    display.setCursor(8,40); display.print("Hold 3s to Start");
    return;
  }
  if (gameOver) {
    display.setCursor(30,28); display.print("GAME OVER");
    display.setCursor(8,40); display.print("Hold 3s to Retry");
    return;
  }

  int gap = 28;
  display.fillRect(pipeX,0,8,pipeGapY,SSD1306_WHITE);
  display.fillRect(pipeX,pipeGapY+gap,8,64,SSD1306_WHITE);

  display.fillRect(20,(int)flapY,YETI_SIZE,YETI_SIZE,SSD1306_WHITE);

  flapVel += 0.6;
  flapY += flapVel;

  int speed = min(6, 3 + flapScore/6);
  pipeX -= speed;

  if(pipeX < -8){
    pipeX = 128;
    pipeGapY = random(8, 28);
    flapScore++;
  }

  bool hitPipe = (20+YETI_SIZE>pipeX && 20<pipeX+8) &&
                 (flapY < pipeGapY || flapY+YETI_SIZE > pipeGapY+gap);

  if(hitPipe || flapY<0 || flapY>H){
    gameRunning=false; gameOver=true;
  }
}

/* ================= TOUCH ================= */
void handleTouch() {
  bool now = digitalRead(TOUCH_PIN);

  // DOUBLE TAP ALWAYS FIRST
  if (now && !lastTouch) {
    unsigned long t = millis();
    tapCount++;

    if (tapCount == 1) lastTapTime = t;
    else if (tapCount == 2 && t-lastTapTime < 400) {
      currentScreen = (ScreenMode)((currentScreen+1)%5);
      resetGames();
      tapCount=0;
      lastTouch=now;
      return;
    }
  }

  // SINGLE TAP
  if (tapCount==1 && millis()-lastTapTime>400) {
    if(currentScreen==SCREEN_YETI)
      currentMood=(Mood)((currentMood+1)%3);
    tapCount=0;
  }

  // GAME INPUT
  if(currentScreen==SCREEN_GAME_JUMP || currentScreen==SCREEN_GAME_FLAP){
    if(now && !lastTouch && !gameRunning)
      gameStartHold = millis();

    if(now && !gameRunning && millis()-gameStartHold>3000){
      resetGames();
      gameRunning=true;
    }

    if(!now && lastTouch && gameRunning){
      if(currentScreen==SCREEN_GAME_JUMP) jumpVel=-8;
      if(currentScreen==SCREEN_GAME_FLAP) flapVel=-9;
    }
  }

  lastTouch = now;
}

/* ================= SETUP ================= */
void setup() {
  pinMode(TOUCH_PIN, INPUT);
  Wire.begin(SDA_PIN,SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC,OLED_ADDR);
  display.setTextColor(SSD1306_WHITE);

  WiFi.begin(WIFI_SSID,WIFI_PASS);
  while(WiFi.status()!=WL_CONNECTED) delay(300);
  configTime(IST_OFFSET,0,ntpServer);
}

/* ================= LOOP ================= */
void loop() {
  if(millis()-lastBlink>random(4000,6000)){
    blinkNow=true; lastBlink=millis();
  }

  display.clearDisplay();

  if(currentScreen==SCREEN_YETI){
    drawEyes(currentMood,blinkNow);
    drawDigitalClock();
  }
  else if(currentScreen==SCREEN_ANALOG){
    drawDigitalClock();
  }
  else if(currentScreen==SCREEN_PLUM){
    drawPlumLayers();
  }
  else if(currentScreen==SCREEN_GAME_JUMP){
    drawYetiJump();
  }
  else if(currentScreen==SCREEN_GAME_FLAP){
    drawYetiFlap();
  }

  display.display();

  if(blinkNow){ delay(120); blinkNow=false; }

  handleTouch();
  delay(60);
}
