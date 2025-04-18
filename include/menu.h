#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "image_loader.h"
#include "ui.h"
#include "led.h"

// Forward declarations
class UI;
class Menu;

enum MenuType
{
    NONE,
    CONFIGURATION,
    MAIN_MENU,
    MAIN_MENU_REORDER,
    MAIN_MENU_PROMPT_REORDER,
    MAIN_MENU_PROMPT_REORDER_AUTO,
    SELECT_BAG,
    LOADING_BAG_CONFIRM,
    STORE,
    STORE_ORDERS,
    STORE_BROWSE, // these choosing menus should really be the same thing
};

enum MenuButton
{
    LEFT,
    MIDDLE,
    RIGHT
};

struct MenuItem
{
    bool visible;
    String text;
    const char *imagePath;
    uint16_t color;
};

class Menu
{
private:
    TFT_eSPI &tft;
    UI &ui;
    ImageLoader &imageLoader;
    LedStrip &ledStrip;

    bool tainted = false;

    MenuItem menuItems[3] = {
        {true, "TL", "/up.png", 0xBDD8},
        {true, "TM", "/dot.png", 0xBDD8},
        {true, "TR", "/down.png", 0xBDD8},
    };

    void handlePressConfiguration(int buttonPin);
    void handlePressMainMenu(int buttonPin);
    void handlePressMainMenuPromptReorder(int buttonPin);
    void handlePressLoadingBagConfirm(int buttonPin);
    void handlePressSelectBag(int buttonPin);
    void handlePressStore(int buttonPin);
    void handlePressStoreOrders(int buttonPin);
    void handlePressStoreBrowse(int buttonPin);

public:
    static const uint16_t menuClearance = 80;

    Menu(TFT_eSPI &tftDisplay, UI &uiInstance, ImageLoader &imageLoaderInstance, LedStrip &ledStrip);
    MenuType current = NONE;

    void begin();
    void clearButtons();
    bool checkButtonEvents();

    void showButton(MenuButton button) { menuItems[button].visible = true; };
    void hideButton(MenuButton button) { menuItems[button].visible = false; };
    bool isButtonVisible(MenuButton button) { return menuItems[button].visible; };

    void draw();
    void redraw()
    {
        tainted = true;
        draw();
    };
    void taint() { tainted = true; }
    void selectMenu(MenuType menuType, bool shouldDraw = true);

    void IRAM_ATTR handlePress(int buttonPin);

    static void IRAM_ATTR handleButtonPress(void *arg);
};

#endif