#include "bag_select.h"

BagSelect::BagSelect(TFT_eSPI &tftDisplay, UI &uiInstance, LedStrip &ledStrip)
    : tft(tftDisplay), ui(uiInstance),
      ledStrip(ledStrip)
{
}

void BagSelect::draw()
{
    if (!needsRedraw)
    {
        return;
    }
    needsRedraw = false;

    tft.fillRect(0, Menu::menuClearance - 24, tft.width(), tft.height() - Menu::menuClearance, BACKGROUND_COLOR);
    ui.menu->redraw();

    tft.setFreeFont(&GeistMono_VariableFont_wght10pt7b);
    tft.setTextColor(PREVIEW_COLOR);

    const uint16_t yPos = Menu::menuClearance - 28;

    if (selectedBagIndex > 0)
    {
        tft.setCursor(4, yPos);

        String text = bags[selectedBagIndex - 1];
        if (text.length() > 10)
        {
            text = text.substring(0, 6) + "..";
        }

        tft.print(text);
    }
    if (selectedBagIndex < bags.size() - 1)
    {
        String text = bags[selectedBagIndex + 1];
        if (text.length() > 10)
        {
            text = text.substring(0, 6) + "..";
        }
        tft.setCursor(tft.width() - tft.textWidth(text) - 4, yPos);

        tft.print(text);
    }

    ui.setIdealFont(bags[selectedBagIndex].c_str(), 30);

    tft.setTextColor(SELECTED_COLOR);
    tft.setCursor(20, tft.height() / 2 + 20);
    tft.print(bags[selectedBagIndex]);

    tft.setTextColor(TEXT_COLOR);
    tft.setFreeFont(&GeistMono_VariableFont_wght10pt7b);
    tft.setCursor(20, tft.height() / 2 + 20 + 40);
    tft.print("12oz bag");
}

void BagSelect::confirmBagSelection()
{
    ledStrip.turnOff();
    scaleManager->loadBag(bags[selectedBagIndex]);
}

void BagSelect::cancelBagSelection()
{
    ledStrip.turnOff();
    tft.fillScreen(BACKGROUND_COLOR);

    ui.menu->selectMenu(MAIN_MENU);
    ui.menu->taint();
    ui.menu->draw();
}

bool BagSelect::selectNextBag()
{
    if (selectedBagIndex < bags.size() - 1)
    {
        selectedBagIndex++;
        needsRedraw = true;

        return true;
    }
    return false;
}

bool BagSelect::selectPreviousBag()
{
    if (selectedBagIndex > 0)
    {
        selectedBagIndex--;
        needsRedraw = true;
        return true;
    }
    return false;
}

void BagSelect::drawProgress()
{
    float progress = selectedBagIndex / (float)bags.size() - 1;
    if (progress < 0.1)
    {
        progress = 0.1;
    }

    ledStrip.progress(progress, RgbColor(255 / 4, 94 / 4, 0));
}