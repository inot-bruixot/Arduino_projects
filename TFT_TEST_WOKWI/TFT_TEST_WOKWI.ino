#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// ── Geometry ──────────────────────────────────────────────────────────
#define CX      120   // horizontal center of the thermometer (240px screen)
#define T_HW     11   // tube half-width  → tube spans CX±11 (22px wide)
#define T_TOP    62   // tube top Y
#define T_BOT   262   // tube bottom Y  (200px height = 5px per °C, 10px per 2°C)
#define B_CY    283   // bulb center Y
#define B_R      17   // bulb outer radius  (283+17=300, safely inside 320px)
#define B_IR     12   // fluid radius inside bulb

#define FL_X  (CX - 8)         // fluid column left X = 112
#define FL_W   16               // fluid column width  (x=112 to x=127)
#define FL_H  (T_BOT - T_TOP)  // 200px total fill height
// ──────────────────────────────────────────────────────────────────────

// ── Colours (initialised in setup with tft.color565) ──────────────────
uint16_t COL_WOOD, COL_GRAIN, COL_CREAM, COL_EDGE;
uint16_t COL_GD, COL_GM, COL_GL;    // glass: dark / mid / lite
uint16_t COL_RD, COL_RM, COL_RL;    // fluid: dark / main / lite
uint16_t COL_TXT;
// ──────────────────────────────────────────────────────────────────────

float temperature = -1;   // -1 forces a draw on first loop

// ── Dark wood background with horizontal grain lines ───────────────────
void drawWood() {
  tft.fillScreen(COL_WOOD);
  for (int y = 7; y < 320; y += 18) {
    tft.drawFastHLine(0, y,   240, COL_GRAIN);
    tft.drawFastHLine(0, y+1, 240, COL_GRAIN);
  }
}

// ── Ivory mounting card (the white panel the thermometer is printed on) ─
void drawPanel() {
  tft.fillRoundRect(58,  8, 124, 306, 8, COL_CREAM);
  tft.drawRoundRect(58,  8, 124, 306, 8, COL_EDGE);
  tft.drawRoundRect(59,  9, 122, 304, 7, COL_EDGE);  // double border
}

// ── Glass tube walls — drawn once, never redrawn ───────────────────────
void drawTubeFrame() {
  // Left wall: 1px dark shadow + 2px mid grey glass
  tft.fillRect(CX - T_HW,     T_TOP, 1, FL_H, COL_GD);
  tft.fillRect(CX - T_HW + 1, T_TOP, 2, FL_H, COL_GM);

  // Interior (air / empty glass): light blue-grey tint
  tft.fillRect(FL_X, T_TOP, FL_W, FL_H, COL_GL);

  // Right wall: 2px mid grey glass + 2px bright white highlight
  tft.fillRect(CX + T_HW - 3, T_TOP, 2, FL_H, COL_GM);
  tft.fillRect(CX + T_HW - 1, T_TOP, 2, FL_H, TFT_WHITE);

  // Rounded sealed top cap
  tft.fillCircle(CX, T_TOP, T_HW,     COL_GM);
  tft.fillCircle(CX, T_TOP, T_HW - 3, COL_GL);
}

// ── Glass bulb at the bottom — drawn once ─────────────────────────────
void drawBulbFrame() {
  tft.fillCircle(CX, B_CY, B_R + 1, COL_GD);  // outer shadow ring
  tft.fillCircle(CX, B_CY, B_R,     COL_GM);  // glass wall
  tft.fillCircle(CX, B_CY, B_R - 2, COL_GL);  // inner glass tint
}

// ── Scale: tick marks + labels — drawn once ────────────────────────────
void drawScale() {
  int x0 = CX + T_HW + 1;   // right edge of tube + 1px gap
  tft.setTextSize(1);
  tft.setTextColor(COL_TXT, COL_CREAM);

  for (int deg = 0; deg <= 40; deg += 2) {
    int y   = map(deg, 0, 40, T_BOT, T_TOP);
    bool big = (deg % 10 == 0);
    tft.drawFastHLine(x0, y, big ? 14 : 7, COL_TXT);
    if (big) {
      tft.setCursor(x0 + 17, y - 3);
      tft.print(deg);
    }
  }
}

// ── Large temperature value at top of ivory panel ─────────────────────
void drawTempValue(float temp) {
  tft.fillRect(59, 9, 122, 40, COL_CREAM);   // erase previous value
  char buf[8];
  sprintf(buf, "%2.0f C", temp);
  int tw = strlen(buf) * 18;                  // textSize=3 → 18px per char
  tft.setTextSize(3);
  tft.setTextColor(COL_TXT, COL_CREAM);
  tft.setCursor(CX - tw / 2, 15);
  tft.print(buf);
}

// ── Update fluid level — called every 2 seconds ────────────────────────
void drawFill(float temp) {
  int fillH  = map((int)temp, 0, 40, 0, FL_H);
  int emptyH = FL_H - fillH;
  int fy     = T_TOP + emptyH;   // Y where fluid starts

  // Empty section above fluid (air/glass look)
  if (emptyH > 0)
    tft.fillRect(FL_X, T_TOP, FL_W, emptyH, COL_GL);

  // Fluid column with 3D shading (dark left edge, main body, bright right edge)
  if (fillH > 0) {
    tft.fillRect(FL_X,             fy, 1,        fillH, COL_RD);
    tft.fillRect(FL_X + 1,         fy, FL_W - 3, fillH, COL_RM);
    tft.fillRect(FL_X + FL_W - 2,  fy, 2,        fillH, COL_RL);
  }

  // Bulb fluid
  tft.fillCircle(CX, B_CY, B_IR,     COL_RD);
  tft.fillCircle(CX, B_CY, B_IR - 1, COL_RM);
  // Glass reflections on the bulb surface (drawn on top of fluid)
  tft.fillCircle(CX - 4, B_CY - 5, 4, COL_RL);                     // upper highlight
  tft.fillCircle(CX + 5, B_CY + 5, 3, tft.color565(120, 0, 0));    // lower shadow

  drawTempValue(temp);
}

// ── setup ──────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);       // portrait: 240 wide × 320 tall
  tft.fillScreen(TFT_BLACK);

  COL_WOOD  = tft.color565( 45,  22,   8);   // dark brown wood
  COL_GRAIN = tft.color565( 60,  30,  12);   // slightly lighter grain lines
  COL_CREAM = tft.color565(235, 220, 185);   // warm ivory panel
  COL_EDGE  = tft.color565(160, 140, 100);   // panel border
  COL_GD    = tft.color565( 20,  20,  35);   // glass dark edge/shadow
  COL_GM    = tft.color565(110, 120, 145);   // glass wall (mid grey-blue)
  COL_GL    = tft.color565(195, 208, 225);   // glass interior (light blue-grey)
  COL_RD    = tft.color565(155,   0,   0);   // fluid dark (left shadow)
  COL_RM    = tft.color565(210,  15,  15);   // fluid main red
  COL_RL    = tft.color565(255, 100,  80);   // fluid highlight (right edge)
  COL_TXT   = tft.color565( 40,  20,   5);   // dark brown text

  drawWood();
  drawPanel();
  drawTubeFrame();
  drawBulbFrame();
  drawScale();
  drawFill(0);

  lastUpdate = millis();
}

// ── loop ───────────────────────────────────────────────────────────────
void loop() {
  if (millis() - lastUpdate >= 2000) {
    lastUpdate = millis();
    temperature = (float)random(0, 41);
    drawFill(temperature);
    Serial.print("Temp: ");
    Serial.print(temperature, 0);
    Serial.println(" C");
  }
}
