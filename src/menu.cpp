#include "menu.h"
#include "buttons.h"
#include "scale.h"
#include "bag_select.h"
#include "store.h"

// Static instance pointer for interrupt handlers
static Menu *instance = nullptr;
struct ButtonInfo
{
    int pin;
};

// Use static objects so they persist after the constructor finishes
static ButtonInfo bitl = {PIN_TOPLEFT};
static ButtonInfo bitm = {PIN_TOPMIDDLE};
static ButtonInfo bitr = {PIN_TOPRIGHT};
static ButtonInfo bitb = {PIN_TERMINAL_BUTTON};

// Added debounce variables
constexpr unsigned long DEBOUNCE_TIME_MS = 200;                      // 200ms debounce time
static volatile unsigned long lastInterruptTimes[4] = {0, 0, 0, 0};  // One per button
static volatile bool buttonEvents[4] = {false, false, false, false}; // Button event flags

// Reference to scale manager (declared extern since it's defined in main.cpp)
extern Scale scaleManager;

void IRAM_ATTR Menu::handleButtonPress(void *arg)
{
    if (!instance)
        return;

    ButtonInfo *btnInfo = (ButtonInfo *)arg;
    int buttonIndex;

    // Convert pin to array index
    if (btnInfo->pin == PIN_TOPLEFT)
        buttonIndex = 0;
    else if (btnInfo->pin == PIN_TOPMIDDLE)
        buttonIndex = 1;
    else if (btnInfo->pin == PIN_TOPRIGHT)
        buttonIndex = 2;
    else if (btnInfo->pin == PIN_TERMINAL_BUTTON)
        buttonIndex = 3;
    else
        return;

    // Check if enough time has passed since the last interrupt
    unsigned long currentTime = millis();
    if (currentTime - lastInterruptTimes[buttonIndex] > DEBOUNCE_TIME_MS)
    {
        lastInterruptTimes[buttonIndex] = currentTime;
        buttonEvents[buttonIndex] = true; // Set flag for processing in the loop
    }
}

// Constructor
Menu::Menu(TFT_eSPI &tftDisplay, UI &uiInstance, ImageLoader &imageLoader)
    : tft(tftDisplay), ui(uiInstance), imageLoader(imageLoader)
{

    // Initialize menu type
    current = MAIN_MENU;
}

void Menu::begin()
{
    // Store instance pointer for interrupt handlers
    instance = this;

    pinMode(PIN_TOPLEFT, INPUT_PULLUP);
    pinMode(PIN_TOPMIDDLE, INPUT_PULLUP);
    pinMode(PIN_TOPRIGHT, INPUT_PULLUP);
    pinMode(PIN_TERMINAL_BUTTON, INPUT_PULLUP);

    attachInterruptArg(PIN_TOPLEFT, handleButtonPress, &bitl, FALLING);
    attachInterruptArg(PIN_TOPMIDDLE, handleButtonPress, &bitm, FALLING);
    attachInterruptArg(PIN_TOPRIGHT, handleButtonPress, &bitr, FALLING);
    attachInterruptArg(PIN_TERMINAL_BUTTON, handleButtonPress, &bitb, FALLING);
}

// Process any pending button events - call this from loop()
bool Menu::checkButtonEvents()
{
    // Check if any button events occurred
    bool eventOccurred = false;

    // Check and process button events
    if (buttonEvents[0])
    {
        eventOccurred = true;
        buttonEvents[0] = false;
        handlePress(PIN_TOPLEFT);
    }
    if (buttonEvents[1])
    {
        eventOccurred = true;
        buttonEvents[1] = false;
        handlePress(PIN_TOPMIDDLE);
    }
    if (buttonEvents[2])
    {
        eventOccurred = true;
        buttonEvents[2] = false;
        handlePress(PIN_TOPRIGHT);
    }
    if (buttonEvents[3])
    {
        eventOccurred = true;
        buttonEvents[3] = false;
        handlePress(PIN_TERMINAL_BUTTON);
    }

    return eventOccurred;
}

void Menu::handlePress(int buttonPin)
{

    switch (current)
    {
    case MAIN_MENU:
        handlePressMainMenu(buttonPin);
        break;
    case SELECT_BAG:
        handlePressSelectBag(buttonPin);
        break;
    case LOADING_BAG_CONFIRM:
        handlePressLoadingBagConfirm(buttonPin);
        break;
    case STORE:
        handlePressStore(buttonPin);
        break;
    case STORE_ORDERS:
        handlePressStoreOrders(buttonPin);
        break;
    case STORE_BROWSE:
        handlePressStoreBrowse(buttonPin);
        break;
    default:
        Serial.println("Unknown menu type");
        break;
    }

    clearButtons();
}

void Menu::selectMenu(MenuType menuType, bool shouldDraw)
{
    // make sure to clear any UI cursor
    ui.stopBlinking();

    if (current == menuType)
    {
        return;
    }

    if (
        (current == STORE || current == STORE_ORDERS || current == STORE_BROWSE) &&
        (menuType != STORE && menuType != STORE_ORDERS && menuType != STORE_BROWSE))
    {
        ui.store->reset();
    }

    // Set the current menu type
    current = menuType;

    switch (menuType)
    {
    case MAIN_MENU:
        menuItems[0].visible = true;
        menuItems[0].imagePath = "/dot.png";
        menuItems[0].text = "Load Bag";

        menuItems[1].visible = false;

        menuItems[2].visible = true;
        menuItems[2].imagePath = "/dot.png";
        menuItems[2].text = "Calibrate";
        break;
    case SELECT_BAG:
        menuItems[0].visible = true;
        menuItems[0].imagePath = "/left.png";
        menuItems[0].text = "";

        menuItems[1].visible = true;
        menuItems[1].imagePath = "/dot.png";
        menuItems[1].text = "Select";

        menuItems[2].visible = true;
        menuItems[2].imagePath = "/right.png";
        menuItems[2].text = "";
        break;
    case LOADING_BAG_CONFIRM:
        menuItems[0].visible = true;
        menuItems[0].imagePath = "/up.png";
        menuItems[0].text = "Confirm";

        menuItems[1].visible = false;

        menuItems[2].visible = true;
        menuItems[2].imagePath = "/down.png";
        menuItems[2].text = "Retake";
        break;
    case STORE:
        menuItems[0].visible = true;
        menuItems[0].imagePath = "/dot.png";
        menuItems[0].text = "Buy";

        menuItems[1].visible = true;
        menuItems[1].imagePath = "/dot.png";
        menuItems[1].text = "Home";

        menuItems[2].visible = true;
        menuItems[2].imagePath = "/dot.png";
        menuItems[2].text = "Orders";
        break;
    case STORE_ORDERS:
        menuItems[0].visible = true;
        menuItems[0].imagePath = "/left.png";
        menuItems[0].text = "Previous";

        menuItems[1].visible = true;
        menuItems[1].imagePath = "/dot.png";
        menuItems[1].text = "Back";

        menuItems[2].visible = true;
        menuItems[2].imagePath = "/right.png";
        menuItems[2].text = "Next";
        break;
    case STORE_BROWSE:
        menuItems[0].visible = true;
        menuItems[0].imagePath = "/left.png";
        menuItems[0].text = "Previous";

        menuItems[1].visible = true;
        menuItems[1].imagePath = "/dot.png";
        menuItems[1].text = "Back";

        menuItems[2].visible = true;
        menuItems[2].imagePath = "/right.png";
        menuItems[2].text = "Next";
    default:
        Serial.println("Unknown Menu Selected");
        break;
    }

    if (shouldDraw)
    {
        tainted = true;
    }

    draw();
}

void Menu::handlePressMainMenu(int buttonPin)
{
    switch (buttonPin)
    {
    case PIN_TOPLEFT:
        scaleManager.startLoadBag();
        break;
    case PIN_TOPRIGHT:
        // Request calibration instead of directly calling calibrate()
        // This is safe to call from an interrupt context
        scaleManager.requestCalibration();
        break;
    case PIN_TERMINAL_BUTTON:
        selectMenu(MenuType::STORE);
        ui.store->taint();
        break;
    default:
        Serial.println("Unknown Button Pressed");
        break;
    }
}

void Menu::handlePressSelectBag(int buttonPin)
{
    switch (buttonPin)
    {
    case PIN_TOPLEFT:
        ui.bagSelect->selectPreviousBag();
        break;
    case PIN_TOPMIDDLE:
        ui.bagSelect->confirmBagSelection();
        break;
    case PIN_TOPRIGHT:
        ui.bagSelect->selectNextBag();
        break;
    default:
        Serial.println("Unknown Button Pressed");
        break;
    }
}

void Menu::handlePressLoadingBagConfirm(int buttonPin)
{
    switch (buttonPin)
    {
    case PIN_TOPLEFT:
        scaleManager.confirmLoadBag();
        break;
    case PIN_TOPRIGHT:
        scaleManager.loadBag(scaleManager.bagName);
        break;
    default:
        Serial.println("Unknown Button Pressed");
        break;
    }
}

void Menu::handlePressStore(int buttonPin)
{
    switch (buttonPin)
    {
    case PIN_TOPMIDDLE:
        ui.store->reset();
        this->selectMenu(MAIN_MENU);
        break;
    case PIN_TOPRIGHT:
        this->selectMenu(STORE_ORDERS);
        ui.store->taint();
        break;
    default:
        Serial.println("Unknown Button Pressed");
        break;
    }
}

void Menu::handlePressStoreOrders(int buttonPin)
{
    switch (buttonPin)
    {
    case PIN_TOPLEFT:
        ui.store->previousOrder();
        break;
    case PIN_TOPMIDDLE:
        this->selectMenu(STORE);
        ui.store->taint();
        break;
    case PIN_TOPRIGHT:
        ui.store->nextOrder();
        break;
    default:
        Serial.println("Unknown Button Pressed");
        break;
    }
}

void Menu::handlePressStoreBrowse(int buttonPin)
{
    switch (buttonPin)
    {
    case PIN_TOPLEFT:
        ui.store->previousProduct();
        break;
    case PIN_TOPMIDDLE:
        this->selectMenu(STORE);
        ui.store->taint();
        break;
    case PIN_TOPRIGHT:
        ui.store->nextProduct();
        break;
    case PIN_TERMINAL_BUTTON:
        ui.store->buyProduct();
        break;
    default:
        Serial.println("Unknown Button Pressed");
        break;
    }
}

void Menu::draw()
{

    if (!tainted)
    {
        return;
    }

    Serial.printf("drawing menu %d\n", current);

    tainted = false;

    // Clear the top menu area
    tft.fillRect(0, 0, tft.width(), 60, BACKGROUND_COLOR);

    // Define positions for the menu icons
    const int16_t iconWidth = 24;  // Estimated width of icons
    const int16_t iconHeight = 24; // Estimated height of icons
    const int16_t iconY = 20;      // Fixed Y position for all icons

    for (int i = 0; i < 3; i++)
    {
        MenuItem &item = menuItems[i];

        if (!item.visible)
            continue;

        // Load the image for the menu item
        const char *imagePath = item.imagePath;

        uint16_t imageWidth, imageHeight;
        const int16_t width = tft.width() / 3;
        const int16_t itemX = i * width;
        const int16_t itemY = 4;
        // default to center of the item
        uint16_t centerX = itemX + (width - iconWidth) / 2;

        if (imagePath)
        {
            Serial.println("Image path is null");

            if (!imageLoader.getImageInfo(imagePath, imageWidth, imageHeight))
            {
                Serial.printf("Image not found: %s\n", imagePath);
                continue;
            }

            const int16_t centerX = itemX + (width - imageWidth) / 2;

            // Draw the icon
            imageLoader.drawPNG(imagePath, centerX, itemY);
        }

        // Draw the text below the icon
        tft.setTextColor(TEXT_COLOR);
        tft.setTextSize(1);
        tft.setFreeFont(&GeistMono_VariableFont_wght10pt7b);

        Serial.printf("text=%s x=%d y=%d\n", item.text.c_str(), centerX, iconY + iconHeight + 10);
        tft.setCursor(centerX + (imageWidth - tft.textWidth(item.text.c_str())) / 2, iconY + iconHeight + 10);
        tft.print(item.text);
    }
}

void Menu::clearButtons()
{
    buttonEvents[0] = false;
    buttonEvents[1] = false;
    buttonEvents[2] = false;
}
