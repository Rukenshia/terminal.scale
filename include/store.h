#ifndef STORE_H
#define STORE_H

#include <Arduino.h>
#include "ui.h"
#include "scale.h"
#include "terminal_api.h"

class Store
{
private:
    UI &ui;
    TFT_eSPI &tft;
    Scale &scaleManager;
    TerminalApi &terminalApi;
    LedStrip &ledStrip;

    bool tainted = false;

    std::vector<Order> orders;
    bool ordersLoaded = false;
    uint orderIndex = 0;
    void drawOrders();
    void loadOrders();

    void recalcOrdersMenuButtons();

public:
    Store(UI &uiInstance, TFT_eSPI &tftDisplay, Scale &scaleInstance, TerminalApi &terminalApi, LedStrip &ledStrip)
        : ui(uiInstance), tft(tftDisplay), scaleManager(scaleInstance), terminalApi(terminalApi), ledStrip(ledStrip) {}

    void exit();
    void taint() { tainted = true; };
    void draw();

    void nextOrder();
    void previousOrder();

    void reset()
    {
        orders = {};
        orderIndex = 0;
        ordersLoaded = false;
    };
};

#endif