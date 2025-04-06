#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Adafruit_GFX.h>

#include "GeistMonoVariableFont_wght18.h"
#include "GeistMonoVariableFont_wght16.h"
#include "GeistMonoVariableFont_wght12.h"

#define ACCENT_COLOR 0x02ff
#define PRIMARY_COLOR TFT_WHITE
#define BACKGROUND_COLOR TFT_BLACK

// Text bounds structure to store position and dimensions of typed text
struct TextBounds
{
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    int16_t cursorX;
    int16_t cursorY;
    int16_t cursorWidth;
    int16_t cursorHeight;
};

struct BlinkState
{
    bool isVisible;
    unsigned long lastToggleTime;
    unsigned long blinkInterval;
    TextBounds bounds;
    TFT_eSPI &tft;
};

// Typing animation that displays text letter by letter with a cursor
TextBounds
typeText(TFT_eSPI &tft, const char *text, const GFXfont *font, int delay_ms = 150, int16_t x = -1, int16_t y = -1);

// Wipe animation for text that was displayed with typeText
void wipeText(TFT_eSPI &tft, const TextBounds &bounds, bool toLeft = true, int speed_ms = 1);

void terminalAnimation(TFT_eSPI &tft);

// Start blinking cursor in the background using the given text bounds
void startBlinking(TFT_eSPI &tft, const TextBounds &bounds, unsigned long blinkInterval = 500);

// Stop blinking cursor
void stopBlinking();

// Get current blink state
bool isBlinking();

#endif