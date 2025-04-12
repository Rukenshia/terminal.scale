#include "ui.h"
#include "scale.h"

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
UI::UI(TFT_eSPI &tftDisplay, LedStrip *ledStrip)
    : tft(tftDisplay),
      ledStrip(ledStrip),
      imageLoader(tftDisplay),
      cursorBlinkTaskHandle(NULL),
      blinkState(NULL)
{
    this->menu = new Menu(tftDisplay, *this);
    // Initialize lastCursorState to zeros
    memset(&lastCursorState, 0, sizeof(TextBounds));
}

// Initialize UI
void UI::begin(Scale *scaleManager)
{
    this->scaleManager = scaleManager;
    menu->begin();

    // Initialize the image loader and mount LittleFS
    if (!imageLoader.begin())
    {
        Serial.println("Failed to initialize image loader");
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
    tft.setFreeFont(config.font);
    tft.setTextSize(1);

    const int textLength = strlen(text);
    const int16_t cursorWidth = tft.textWidth("W");

    // Calculate font height - set a reasonable default if needed
    int16_t fontHeight = 32; // Default in case we can't get actual height
    if (config.font && config.font->yAdvance > 0)
    {
        fontHeight = config.font->yAdvance;
    }

    // get min char width and space width
    const int16_t spaceWidth = tft.textWidth("a b") - tft.textWidth("ab");
    const int16_t cursorHeight = fontHeight;

    // Calculate text width for centering
    int16_t totalTextWidth = tft.textWidth(text);

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

    const int16_t cursorY = textY - cursorHeight + 6;
    int16_t cursorX = startX;
    const uint8_t cursorPadding = 4;

    TextBounds bounds;

    // Type out the text character by character
    for (int i = 0; i < textLength; i++)
    {
        // If cursor is visible, clear it before drawing the new character
        if (i > 0 || config.enableCursor)
        {
            tft.fillRect(cursorX, cursorY, cursorWidth + cursorPadding, cursorHeight, BACKGROUND_COLOR);
        }

        // Calculate width of current character
        int16_t singleCharWidth;

        // Handle space and non-space characters differently
        if (text[i] == ' ')
        {
            // For spaces, just advance the cursor position without trying to render anything
            singleCharWidth = spaceWidth;
        }
        else
        {
            // For visible characters, render them and calculate their width
            char currentChar[2] = {text[i], '\0'};
            singleCharWidth = tft.textWidth(currentChar);

            // Set cursor position and draw only this character
            tft.setCursor(cursorX, textY);
            tft.print(currentChar);
        }

        // Update cursor position for next character
        cursorX += singleCharWidth;

        // Draw the cursor after the current character
        if (i < textLength - 1 || config.enableCursor)
        {
            tft.fillRect(cursorX + cursorPadding, cursorY, cursorWidth, cursorHeight, config.cursorColor);
        }

        // Create and return the TextBounds structure
        bounds.x = startX;
        bounds.y = textY;
        bounds.width = totalTextWidth;
        bounds.height = fontHeight;
        bounds.cursorX = cursorX + cursorPadding;
        bounds.cursorY = cursorY;
        bounds.cursorWidth = cursorWidth;
        bounds.cursorHeight = cursorHeight;
        bounds.yAdvance = config.font ? config.font->yAdvance : 0;

        // Update the last cursor state
        lastCursorState = bounds;

        delay(config.delay_ms);
    }

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
    ledStrip->turnOnAnimation();
    tft.fillScreen(BACKGROUND_COLOR);

    // Use the typeText function with the original parameters
    const char *text = "terminal";
    auto config = createTextConfig(&GeistMono_VariableFont_wght18pt7b);
    TextBounds bounds = typeText(text, config);

    delay(1500);
    ledStrip->turnOffAnimation();
    delay(500);

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
    BaseType_t result = xTaskCreatePinnedToCore(
        cursorBlinkTaskWrapper, // Function that implements the task
        "CursorBlink",          // Text name for the task
        4096,                   // Stack size in words
        (void *)blinkState,     // Parameter passed into the task
        8,                      // Priority of the task (1 is low)
        &cursorBlinkTaskHandle, // Task handle
        1                       // needs to be pinned to core 1 because we are drawing on the display
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
    menu->draw();
}

void UI::loop()
{
    drawMenu();

    if (scaleManager->bagRemovedFromSurface)
    {
        if (!drawnBagNotFound)
        {
            unsigned long currentTime = millis();
            auto textConfig = createTextConfig(&GeistMono_VariableFont_wght14pt7b);
            textConfig.y = tft.height() / 2 + titleText.font->yAdvance + 8;
            textConfig.enableCursor = false;
            // FIXME: should probably be a background task?
            tft.fillRect(0, tft.height() / 2 - titleText.font->yAdvance - 8,
                         tft.width(), tft.height(), BACKGROUND_COLOR);
            auto bounds = typeText("404", titleText);
            typeText("Bag not found", textConfig);
            startBlinkingAt(bounds);

            drawnBagNotFound = true;
        }

        return;
    }

    if (drawnBagNotFound)
    {
        drawnBagNotFound = false;
        tft.fillRect(0, tft.height() / 2 - titleText.font->yAdvance - 8,
                     tft.width(), tft.height(), BACKGROUND_COLOR);
        stopBlinking();
    }

    drawWeight(scaleManager->lastReading);
}

void UI::drawWeight(float weight)
{
    // round to nearest 0.1g
    weight = round(weight * 10.0) / 10.0;

    if (weight == lastDrawnReading)
    {
        return;
    }
    lastDrawnReading = weight;

    float progress = max(min(weight / TERMINAL_COFFEE_WEIGHT, 1.0f), 0.0f);
    int barWidth = int(tft.width() * progress);
    int barHeight = 20;

    // Draw the weight in the center of the screen
    tft.setFreeFont(&GeistMono_VariableFont_wght18pt7b);
    tft.setTextColor(TEXT_COLOR);
    tft.setTextSize(1);

    auto text = String(weight, 1) + " g";
    auto w = tft.textWidth(text.c_str());

    // clear weight text area
    tft.fillRect(0, (tft.height() / 2) - (GeistMono_VariableFont_wght18pt7b.yAdvance / 2) - 8,
                 tft.width(), GeistMono_VariableFont_wght18pt7b.yAdvance + 16, BACKGROUND_COLOR);

    tft.setCursor((tft.width() - w) / 2, (tft.height() / 2) + GeistMono_VariableFont_wght18pt7b.yAdvance / 2);
    tft.print(text);

    if (barWidth == lastProgressBarWidth)
    {
        return;
    }
    lastProgressBarWidth = barWidth;

    tft.fillRect(0, tft.height() - barHeight, tft.width(), barHeight, BACKGROUND_COLOR);
    tft.fillRect(0, tft.height() - barHeight, barWidth, barHeight, ACCENT_COLOR);
}