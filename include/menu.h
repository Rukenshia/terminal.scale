#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "image_loader.h"
#include "ui.h"

// Forward declarations
class UI;
class Menu;

enum MenuType
{
    MAIN_MENU,
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
};

class Menu
{
private:
    TFT_eSPI &tft;
    UI &ui;
    ImageLoader &imageLoader;

    bool tainted = false;

    MenuItem menuItems[3] = {
        {true, "TL", "/up.png"},
        {true, "TM", "/dot.png"},
        {true, "TR", "/down.png"},
    };

    void handlePressMainMenu(int buttonPin);
    void handlePressLoadingBagConfirm(int buttonPin);
    void handlePressSelectBag(int buttonPin);
    void handlePressStore(int buttonPin);
    void handlePressStoreOrders(int buttonPin);
    void handlePressStoreBrowse(int buttonPin);

public:
    Menu(TFT_eSPI &tftDisplay, UI &uiInstance, ImageLoader &imageLoaderInstance);
    MenuType current;

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