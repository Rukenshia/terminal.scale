#ifndef BAG_SELECT_H
#define BAG_SELECT_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <vector>

#include "ui.h"
#include "scale.h"

#define PREVIEW_COLOR 0x736C
#define SELECTED_COLOR ACCENT_COLOR

class BagSelect
{
private:
    TFT_eSPI &tft;
    UI &ui;
    Scale *scaleManager;

    std::vector<String> bags;

    uint selectedBagIndex = 0;
    bool needsRedraw = false;

public:
    BagSelect(TFT_eSPI &tftDisplay, UI &uiInstance);
    void begin(Scale *scaleManagerInstance)
    {
        scaleManager = scaleManagerInstance;
    };
    void setBags(const std::vector<String> &bagList) { bags = bagList; };
    void taint() { needsRedraw = true; };
    void draw();
    void selectBag(uint index);
    bool selectNextBag();
    bool selectPreviousBag();
    void confirmBagSelection();
    void cancelBagSelection();
};

#endif