#include "ui.h"

// Global variables for cursor blinking
TaskHandle_t cursorBlinkTaskHandle = NULL;
BlinkState *blinkState = NULL;

// FreeRTOS task for cursor blinking
void cursorBlinkTask(void *parameter)
{
    BlinkState *state = (BlinkState *)parameter;

    while (true)
    {
        // Toggle cursor visibility
        state->isVisible = !state->isVisible;

        if (state->isVisible)
        {
            // Draw cursor
            state->tft.fillRect(state->bounds.cursorX,
                                state->bounds.cursorY,
                                state->bounds.cursorWidth,
                                state->bounds.cursorHeight,
                                ACCENT_COLOR);
        }
        else
        {
            // Erase cursor
            state->tft.fillRect(state->bounds.cursorX,
                                state->bounds.cursorY,
                                state->bounds.cursorWidth,
                                state->bounds.cursorHeight,
                                BACKGROUND_COLOR);
        }

        // Delay for specified interval
        vTaskDelay(state->blinkInterval / portTICK_PERIOD_MS);
    }
}

TextBounds typeText(TFT_eSPI &tft, const char *text, const GFXfont *font, int delay_ms, int16_t x, int16_t y)
{
    tft.setTextColor(PRIMARY_COLOR);
    tft.setTextSize(1);
    tft.setFreeFont(font);

    const int textLength = strlen(text);
    int16_t charWidth = 0;
    const int16_t cursorWidth = 20; // Width of the accent color box

    // Calculate font height - set a reasonable default if needed
    int16_t fontHeight = 32; // Default in case we can't get actual height
    if (font->yAdvance > 0)
    {
        fontHeight = font->yAdvance;
    }

    const int16_t cursorHeight = fontHeight + 8;

    // Calculate text width for centering
    int16_t totalTextWidth = tft.textWidth(text) + cursorWidth + 20;

    // Set coordinates - center if not specified
    int16_t startX = x;
    int16_t textY = y;
    if (startX < 0)
    {
        // Center horizontally
        startX = (tft.width() - totalTextWidth) / 2;
    }
    if (textY < 0)
    {
        // Center vertically
        textY = (tft.height() + fontHeight) / 2;
    }

    const int16_t cursorY = textY - cursorHeight + 4;
    int16_t cursorX = startX;

    // Type out the text
    charWidth = 0;
    for (int i = 0; i < textLength; i++)
    {
        // Get the width of the current character
        charWidth += tft.textWidth(String(text[i]));

        // Clear the previous cursor and text area
        tft.fillRect(startX, cursorY, charWidth + cursorWidth, cursorHeight, BACKGROUND_COLOR);

        // Draw the text we've typed so far
        tft.setCursor(startX, textY);
        for (int j = 0; j < i; j++)
        {
            tft.print(text[j]);
        }

        // Draw the cursor at current position
        cursorX = startX + charWidth + 8;
        tft.fillRect(cursorX, cursorY, cursorWidth, cursorHeight, ACCENT_COLOR);

        delay(delay_ms);
    }

    // Draw final character
    tft.fillRect(cursorX, cursorY, cursorWidth, cursorHeight, BACKGROUND_COLOR);
    tft.setCursor(startX, textY);
    tft.print(text);
    charWidth += tft.textWidth(String(text[textLength - 1]));

    cursorX = startX + charWidth + 8;
    tft.fillRect(cursorX, cursorY, cursorWidth, cursorHeight, ACCENT_COLOR);
    delay(delay_ms); // Wait for the last character to be drawn

    // Create and return the TextBounds structure
    TextBounds bounds;
    bounds.x = startX;
    bounds.y = textY;
    bounds.width = charWidth;
    bounds.height = fontHeight;
    bounds.cursorX = cursorX;
    bounds.cursorY = cursorY;
    bounds.cursorWidth = cursorWidth;
    bounds.cursorHeight = cursorHeight;
    bounds.yAdvance = font->yAdvance;

    return bounds;
}

void wipeText(TFT_eSPI &tft, const TextBounds &bounds, int speed_ms)
{
    int16_t cursorX = bounds.cursorX;
    const int16_t wipeHeight = max(bounds.height, bounds.cursorHeight) + 8;

    // Smoothly sweep the cursor to the left
    while (cursorX > bounds.x)
    {
        tft.drawFastVLine(cursorX + bounds.cursorWidth, bounds.cursorY, wipeHeight, BACKGROUND_COLOR);
        cursorX--;
        tft.drawFastVLine(cursorX, bounds.cursorY, bounds.cursorHeight, ACCENT_COLOR);
        delay(speed_ms);
    }

    // Clear entire text area including cursor at the end
    tft.fillRect(
        bounds.x,
        bounds.cursorY,
        bounds.width + bounds.cursorWidth,
        bounds.yAdvance,
        BACKGROUND_COLOR);
}

void terminalAnimation(TFT_eSPI &tft)
{
    tft.fillScreen(BACKGROUND_COLOR);

    // Use the new typeText function with the original parameters
    const char *text = "terminal";
    TextBounds bounds = typeText(tft, text, &GeistMono_VariableFont_wght18pt7b, 150, 10, 50);

    // Use wipeText for the exit animation
    wipeText(tft, bounds);

    tft.fillScreen(BACKGROUND_COLOR);
}

void startBlinking(TFT_eSPI &tft, const TextBounds &bounds, unsigned long blinkInterval)
{
    // Stop any existing blinking
    stopBlinking();

    // Create new blink state
    blinkState = new BlinkState{true, millis(), blinkInterval, bounds, tft};

    // Create the blinking task
    xTaskCreate(
        cursorBlinkTask,       // Function that implements the task
        "CursorBlink",         // Text name for the task
        4096,                  // Stack size in words
        (void *)blinkState,    // Parameter passed into the task
        1,                     // Priority of the task (1 is low)
        &cursorBlinkTaskHandle // Task handle
    );
}

void stopBlinking()
{
    if (cursorBlinkTaskHandle != NULL)
    {
        // Delete the task
        vTaskDelete(cursorBlinkTaskHandle);
        cursorBlinkTaskHandle = NULL;

        // Clear the cursor if it's currently visible
        if (blinkState != NULL)
        {
            blinkState->tft.fillRect(
                blinkState->bounds.cursorX,
                blinkState->bounds.cursorY,
                blinkState->bounds.cursorWidth,
                blinkState->bounds.cursorHeight,
                BACKGROUND_COLOR);

            delete blinkState;
            blinkState = NULL;
        }
    }
}

bool isBlinking()
{
    return (cursorBlinkTaskHandle != NULL);
}