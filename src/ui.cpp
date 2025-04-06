#include "ui.h"

// Global task wrapper function - needed because FreeRTOS tasks must be regular C functions
void cursorBlinkTaskWrapper(void *parameter)
{
    BlinkState *state = (BlinkState *)parameter;

    // Safety check to prevent null pointer dereference
    if (!state)
    {
        Serial.println("Error: Null state in cursorBlinkTask");
        vTaskDelete(NULL); // Delete the current task
        return;
    }

    // Extract UI instance pointer from the pointer stored in our state
    UI *uiInstance = (UI *)(state->userData);

    // Safety check for UI instance
    if (!uiInstance)
    {
        Serial.println("Error: Null UI instance in cursorBlinkTask");
        vTaskDelete(NULL); // Delete the current task
        return;
    }

    while (true)
    {
        // Toggle cursor visibility
        state->isVisible = !state->isVisible;

        if (state->isVisible)
        {
            // Draw cursor
            uiInstance->tft.fillRect(
                state->bounds.cursorX,
                state->bounds.cursorY,
                state->bounds.cursorWidth,
                state->bounds.cursorHeight,
                ACCENT_COLOR);
        }
        else
        {
            // Erase cursor
            uiInstance->tft.fillRect(
                state->bounds.cursorX,
                state->bounds.cursorY,
                state->bounds.cursorWidth,
                state->bounds.cursorHeight,
                BACKGROUND_COLOR);
        }

        // Delay for specified interval
        vTaskDelay(state->blinkInterval / portTICK_PERIOD_MS);
    }
}

// Constructor
UI::UI(TFT_eSPI &tftDisplay)
    : tft(tftDisplay),
      imageLoader(tftDisplay),
      cursorBlinkTaskHandle(NULL),
      blinkState(NULL)
{
    // Initialize lastCursorState to zeros
    memset(&lastCursorState, 0, sizeof(TextBounds));
}

// Initialize UI
void UI::begin()
{
    // Initialize the image loader and mount LittleFS
    if (!imageLoader.begin())
    {
        Serial.println("Failed to initialize image loader");
    }
    else
    {
        Serial.println("Image loader initialized successfully");
        // List all PNG files to verify they're available
        imageLoader.listPNGFiles();
    }
}

// Create a default text configuration
TextConfig UI::createTextConfig(const GFXfont *font)
{
    return TextConfig(font);
}

// New typing animation with configuration object
TextBounds UI::typeText(const char *text, const TextConfig &config)
{
    tft.setTextColor(config.textColor);
    tft.setTextSize(1);
    tft.setFreeFont(config.font);

    const int textLength = strlen(text);
    int16_t charWidth = 0;
    const int16_t cursorWidth = config.cursorWidth;

    // Calculate font height - set a reasonable default if needed
    int16_t fontHeight = 32; // Default in case we can't get actual height
    if (config.font && config.font->yAdvance > 0)
    {
        fontHeight = config.font->yAdvance;
    }

    const int16_t cursorHeight = fontHeight + 8;

    // Calculate text width for centering
    int16_t totalTextWidth = tft.textWidth(text) + cursorWidth + 20;

    // Set coordinates - center if not specified
    int16_t startX = config.x;
    int16_t textY = config.y;
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
        if (text[i] == ' ')
        {
            // Handle space character
            charWidth += tft.textWidth("_");
            continue;
        }
        else
        {
            charWidth += tft.textWidth(String(text[i]));
        }

        Serial.printf("Drawing character: %c, width: %d\n", text[i], charWidth);

        // Clear the previous cursor and text area
        tft.fillRect(startX, cursorY, charWidth + cursorWidth, cursorHeight, BACKGROUND_COLOR);

        // Draw the text we've typed so far
        tft.setCursor(startX, textY);
        for (int j = 0; j < i; j++)
        {
            tft.print(text[j]);
        }

        // Draw the cursor at current position if enabled
        cursorX = startX + charWidth + 12;

        Serial.printf("Cursor position: %d\n", cursorX);
        if (config.enableCursor)
        {
            tft.fillRect(cursorX, cursorY, cursorWidth, cursorHeight, config.cursorColor);
        }

        delay(config.delay_ms);
    }

    // Draw final character
    charWidth += tft.textWidth(String(text[textLength - 1]));
    Serial.printf("Drawing character: %s, width: %d\n", text, charWidth);
    tft.fillRect(cursorX, cursorY, cursorWidth, cursorHeight, BACKGROUND_COLOR);
    tft.setCursor(startX, textY);
    tft.print(text);

    cursorX = startX + charWidth + 12;
    Serial.printf("Cursor position: %d\n", cursorX);
    if (config.enableCursor)
    {
        tft.fillRect(cursorX, cursorY, cursorWidth, cursorHeight, config.cursorColor);
    }
    delay(config.delay_ms); // Wait for the last character to be drawn

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
    bounds.yAdvance = config.font ? config.font->yAdvance : 0;

    Serial.printf("TextBounds: x=%d, y=%d, width=%d, height=%d, cursorX=%d, cursorY=%d\n",
                  bounds.x, bounds.y, bounds.width, bounds.height, bounds.cursorX, bounds.cursorY);

    // Update the last cursor state
    lastCursorState = bounds;

    return bounds;
}

void UI::wipeText(const TextBounds &bounds, int speed_ms)
{
    int16_t cursorX = bounds.cursorX;
    const int16_t wipeHeight = max(bounds.height, bounds.cursorHeight) + 8;

    // Calculate the full area that needs to be wiped
    int16_t areaStartX = bounds.x;
    int16_t areaWidth = bounds.width + bounds.cursorWidth + 20; // Add extra margin to ensure all text is cleared

    // Smoothly sweep the cursor to the left
    while (cursorX > bounds.x)
    {
        tft.drawFastVLine(cursorX + bounds.cursorWidth, bounds.cursorY, wipeHeight, BACKGROUND_COLOR);
        cursorX--;
        tft.drawFastVLine(cursorX, bounds.cursorY, bounds.cursorHeight, ACCENT_COLOR);
        delay(speed_ms);
    }

    // Clear entire text area including cursor and any possible remnants
    // Use a larger area than strictly necessary to ensure complete cleanup
    tft.fillRect(
        areaStartX - 5,     // Start slightly before text area
        bounds.cursorY - 5, // Start slightly above cursor
        areaWidth + 10,     // Add extra pixels to ensure complete clearing
        wipeHeight + 10,    // Use wipeHeight plus margin for complete vertical clearing
        BACKGROUND_COLOR);
}

void UI::terminalAnimation()
{
    tft.fillScreen(BACKGROUND_COLOR);

    // Use the typeText function with the original parameters
    const char *text = "terminal";
    auto config = createTextConfig(&GeistMono_VariableFont_wght18pt7b);
    TextBounds bounds = typeText(text, config);

    // Use wipeText for the exit animation
    wipeText(bounds);

    tft.fillScreen(BACKGROUND_COLOR);
}

// Start blinking cursor using the last cursor position
void UI::startBlinking(unsigned long blinkInterval)
{
    // Check if we have a valid cursor state
    if (lastCursorState.cursorWidth > 0 && lastCursorState.cursorHeight > 0)
    {
        startBlinkingAt(lastCursorState, blinkInterval);
    }
    else
    {
        // Warning: no previous cursor position available
        Serial.println("Warning: No previous cursor position available for blinking");
    }
}

// Start blinking cursor at a specific position
void UI::startBlinkingAt(const TextBounds &bounds, unsigned long blinkInterval)
{
    // Stop any existing blinking
    stopBlinking();

    // Create new blink state with direct pointer to this object instead of storing in yAdvance
    blinkState = new BlinkState;
    if (blinkState == NULL)
    {
        Serial.println("Error: Failed to allocate BlinkState");
        return;
    }

    // Initialize the blink state
    blinkState->isVisible = true;
    blinkState->lastToggleTime = millis();
    blinkState->blinkInterval = blinkInterval;
    blinkState->bounds = bounds;
    blinkState->userData = this; // Store pointer to UI instance directly

    // Create the blinking task
    BaseType_t result = xTaskCreate(
        cursorBlinkTaskWrapper, // Function that implements the task
        "CursorBlink",          // Text name for the task
        4096,                   // Stack size in words
        (void *)blinkState,     // Parameter passed into the task
        1,                      // Priority of the task (1 is low)
        &cursorBlinkTaskHandle  // Task handle
    );

    if (result != pdPASS)
    {
        Serial.println("Error: Failed to create cursor blink task");
        delete blinkState;
        blinkState = NULL;
    }
}

void UI::stopBlinking()
{
    if (cursorBlinkTaskHandle != NULL)
    {
        // Delete the task
        vTaskDelete(cursorBlinkTaskHandle);
        cursorBlinkTaskHandle = NULL;

        // Clear the cursor if it's currently visible
        if (blinkState != NULL)
        {
            tft.fillRect(
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

bool UI::isBlinking()
{
    return (cursorBlinkTaskHandle != NULL);
}

void UI::drawMenu()
{
    // Clear the left side menu area
    tft.fillRect(0, 0, 100, tft.height(), BACKGROUND_COLOR);

    // Define positions for the menu icons
    const int16_t iconWidth = 24;  // Estimated width of icons
    const int16_t iconHeight = 24; // Estimated height of icons
    const int16_t iconX = 25;      // Center icons in the left menu area

    // Get the actual image dimensions if possible
    uint16_t upWidth, upHeight;
    uint16_t dotWidth, dotHeight;
    uint16_t downWidth, downHeight;

    // Try to get image dimensions - if it fails, use the estimates
    if (imageLoader.getImageInfo(menu.upImage, upWidth, upHeight))
    {
        Serial.printf("Found up icon: %dx%d\n", upWidth, upHeight);
    }
    else
    {
        upWidth = iconWidth;
        upHeight = iconHeight;
        Serial.println("Could not get up icon dimensions");
    }

    if (imageLoader.getImageInfo(menu.selectImage, dotWidth, dotHeight))
    {
        Serial.printf("Found select icon: %dx%d\n", dotWidth, dotHeight);
    }
    else
    {
        dotWidth = iconWidth;
        dotHeight = iconHeight;
        Serial.println("Could not get select icon dimensions");
    }

    if (imageLoader.getImageInfo(menu.downImage, downWidth, downHeight))
    {
        Serial.printf("Found down icon: %dx%d\n", downWidth, downHeight);
    }
    else
    {
        downWidth = iconWidth;
        downHeight = iconHeight;
        Serial.println("Could not get down icon dimensions");
    }

    // Calculate positions to center icons horizontally in left menu area
    const int16_t upX = iconX - (upWidth / 2);
    const int16_t dotX = iconX - (dotWidth / 2);
    const int16_t downX = iconX - (downWidth / 2);

    // Position icons vertically
    const int16_t upY = 20;                                // Top icon
    const int16_t dotY = tft.height() / 2 - dotHeight / 2; // Middle icon
    const int16_t downY = tft.height() - downHeight - 20;  // Bottom icon

    // Draw the icons
    Serial.println("Drawing menu icons");
    imageLoader.drawPNG(menu.upImage, upX, upY);
    imageLoader.drawPNG(menu.selectImage, dotX, dotY);
    imageLoader.drawPNG(menu.downImage, downX, downY);

    // Draw text next to the icons
    tft.setTextColor(TEXT_COLOR);
    tft.setTextSize(1);
    tft.setFreeFont(&GeistMono_VariableFont_wght10pt7b);
    auto fontHeight = tft.fontHeight();

    tft.setCursor(iconX + 30, upY + iconHeight - 14 / 2);
    tft.print(menu.item1Text);

    tft.setCursor(iconX + 30, dotY + iconHeight - 14 / 2);
    tft.print(menu.item2Text);
    tft.setCursor(iconX + 30, downY + iconHeight - 14 / 2);
    tft.print(menu.item3Text);
}