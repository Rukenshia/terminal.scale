#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Adafruit_GFX.h>

#include "GeistMonoVariableFont_wght18.h"

#define ACCENT_COLOR 0x02ff
#define PRIMARY_COLOR TFT_WHITE
#define BACKGROUND_COLOR TFT_BLACK

void terminalAnimation(TFT_eSPI &tft);

#endif