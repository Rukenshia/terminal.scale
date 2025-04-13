#ifndef BAG_SELECT_H
#define BAG_SELECT_H

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "ui.h"

class BagSelect
{
private:
    TFT_eSPI &tft;
    UI &ui;

public:
    BagSelect(TFT_eSPI &tftDisplay, UI &uiInstance);
    void begin();
    void draw();
    void selectBag(const String &bagName);
    void confirmBagSelection();
    void cancelBagSelection();
};

#endif