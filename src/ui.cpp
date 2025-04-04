#include "ui.h"

void terminalAnimation(TFT_eSPI &tft)
{
    tft.fillScreen(BACKGROUND_COLOR);
    tft.setTextColor(PRIMARY_COLOR);
    tft.setTextSize(1);
    tft.setCursor(0, 0);

    tft.setFreeFont(&GeistMono_VariableFont_wght18pt7b);

    // A box of ACCENT_COLOR appears (width of 20px)
    // the text "terminal" starts appearing letter by letter with the box moving in front of it
    // when it is done typing, the ACCENT_COLOR box flashes twice
    // then the text "terminal" disappears letter by letter with the box moving behind it

    const char *text = "terminal ";
    const int textLength = strlen(text);
    int16_t charWidth = 0;          // Current char width
    const int16_t cursorWidth = 20; // Width of the accent color box
    const int16_t startX = 10;
    const int16_t y = 50;
    const int delay_ms = 150; // Delay between characters

    // const int16_t fontHeight = GeistMono_VariableFont_wght18pt7b.yAdvance;
    const int16_t fontHeight = 32; // because there is no `g` etc, the font height is not
                                   // calculated correctly, so we set it manually
    const int16_t cursorHeight = fontHeight + 8;
    const int16_t cursorY = y - cursorHeight + 4;

    Serial.printf("%d %d %d %d\n", tft.textWidth("a"), tft.textWidth("b"), tft.textWidth("c"), tft.textWidth("d"));

    // Type out the text
    for (int i = 0; i <= textLength; i++)
    {
        // Get the width of the current character
        charWidth += tft.textWidth(String(text[i]));

        Serial.printf("charWidth: %d\n", charWidth);

        // Clear the previous cursor and text area
        tft.fillRect(startX, cursorY, charWidth + cursorWidth, cursorHeight, BACKGROUND_COLOR);

        // Draw the text we've typed so far
        tft.setCursor(startX, y);
        for (int j = 0; j < i; j++)
        {
            tft.print(text[j]);
        }

        // Draw the cursor at current position
        int cursorX = startX + charWidth + 8;
        tft.fillRect(cursorX, cursorY, cursorWidth, cursorHeight, ACCENT_COLOR);

        delay(delay_ms);
    }

    Serial.printf("blinking cursor at x: %d\n", startX + charWidth);

    // Cursor blinking twice
    for (int blink = 0; blink < 2; blink++)
    {
        // Hide cursor
        tft.fillRect(startX + charWidth, cursorY, cursorWidth, cursorHeight, BACKGROUND_COLOR);
        delay(300);

        // Show cursor
        tft.fillRect(startX + charWidth, cursorY, cursorWidth, cursorHeight, ACCENT_COLOR);
        delay(300);
    }

    // Smoothly sweep the cursor to the left
    int16_t x = startX + charWidth;
    while (x > startX + 1)
    {
        tft.drawFastVLine(x + cursorWidth, cursorY, cursorHeight, BACKGROUND_COLOR);
        x -= 1;
        tft.drawFastVLine(x, cursorY, cursorHeight, ACCENT_COLOR);

        delay(1);
    }

    // Clear the cursor at the end
    tft.fillRect(x - 1, cursorY, cursorWidth, cursorHeight, BACKGROUND_COLOR);
}