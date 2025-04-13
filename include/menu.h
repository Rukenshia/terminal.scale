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

public:
    Menu(TFT_eSPI &tftDisplay, UI &uiInstance, ImageLoader &imageLoaderInstance);
    MenuType current;

    void begin();
    void clearButtons();
    bool checkButtonEvents();

    void draw();
    void taint() { tainted = true; }
    void selectMenu(MenuType menuType, bool shouldDraw = true);

    void IRAM_ATTR handlePress(int buttonPin);

    static void IRAM_ATTR handleButtonPress(void *arg);
};

#endif