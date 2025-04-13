#include "bag_select.h"

BagSelect::BagSelect(TFT_eSPI &tftDisplay, UI &uiInstance)
    : tft(tftDisplay), ui(uiInstance)
{
}

void BagSelect::draw()
{
    if (!needsRedraw)
    {
        return;
    }
    needsRedraw = false;

    const uint16_t menuClearanceY = 60;

    tft.fillRect(0, menuClearanceY, tft.width(), tft.height() - menuClearanceY, BACKGROUND_COLOR);

    tft.setFreeFont(&GeistMono_VariableFont_wght10pt7b);
    tft.setTextColor(PREVIEW_COLOR);

    const uint16_t yPos = menuClearanceY + 20;

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

    tft.setFreeFont(&GeistMono_VariableFont_wght18pt7b);

    if (tft.textWidth(bags[selectedBagIndex]) > tft.width())
    {
        tft.setFreeFont(&GeistMono_VariableFont_wght14pt7b);
    }

    tft.setTextColor(SELECTED_COLOR);
    tft.setCursor((tft.width() - tft.textWidth(bags[selectedBagIndex])) / 2, yPos + 60);
    tft.print(bags[selectedBagIndex]);
}

void BagSelect::confirmBagSelection()
{
    ui.menu->selectMenu(MAIN_MENU);
    ui.menu->taint();
    ui.menu->draw();
}

void BagSelect::cancelBagSelection()
{
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