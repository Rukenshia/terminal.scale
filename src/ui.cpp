#include "ui.h"
#include "scale.h"
#include "bag_select.h"
#include "store.h"

void cursorBlinkTaskWrapper(void *parameter)
{
    BlinkState *state = (BlinkState *)parameter;

    if (!state)
    {
        Serial.println("Error: Null state in cursorBlinkTask");
        vTaskDelete(NULL);
        return;
    }

    UI *uiInstance = (UI *)(state->userData);

    if (!uiInstance)
    {
        Serial.println("Error: Null UI instance in cursorBlinkTask");
        vTaskDelete(NULL);
        return;
    }

    while (true)
    {
        state->isVisible = !state->isVisible;

        if (state->isVisible)
        {
            uiInstance->tft.fillRect(
                state->bounds.cursorX,
                state->bounds.cursorY,
                state->bounds.cursorWidth,
                state->bounds.cursorHeight,
                ACCENT_COLOR);
        }
        else
        {
            uiInstance->tft.fillRect(
                state->bounds.cursorX,
                state->bounds.cursorY,
                state->bounds.cursorWidth,
                state->bounds.cursorHeight,
                BACKGROUND_COLOR);
        }

        vTaskDelay(state->blinkInterval / portTICK_PERIOD_MS);
    }
}

// Constructor
UI::UI(TFT_eSPI &tftDisplay, LedStrip &ledStrip, TerminalApi &terminalApi, PreferencesManager &preferences)
    : tft(tftDisplay),
      ledStrip(ledStrip),
      imageLoader(tftDisplay),
      cursorBlinkTaskHandle(NULL),
      blinkState(NULL),
      terminalApi(terminalApi),
      preferences(preferences)
{
    this->menu = new Menu(tftDisplay, *this, imageLoader, ledStrip);
    this->bagSelect = new BagSelect(tftDisplay, *this, ledStrip);
    this->store = new Store(*this, tftDisplay, *scaleManager, terminalApi, ledStrip);
    memset(&lastCursorState, 0, sizeof(TextBounds));
}

// Initialize UI
void UI::begin(Scale *scaleManager)
{
    this->scaleManager = scaleManager;
    menu->begin();
    bagSelect->begin(scaleManager);

    if (!imageLoader.begin())
    {
        Serial.println("Failed to initialize image loader");
    }
}

void UI::beginConfiguration()
{
    tft.fillScreen(BACKGROUND_COLOR);
    auto bounds = typeTitle("Configuration");
    delay(2000);
    wipeText(bounds);

    menu->selectMenu(MenuType::CONFIGURATION);
    menu->redraw();

    String text = "Order automatically?";

    auto width = setIdealFont(text.c_str(), 16);

    tft.setCursor((tft.width() - width) / 2, (tft.height() - Menu::menuClearance) / 2 + Menu::menuClearance);
    tft.setTextColor(TEXT_COLOR);
    tft.print(text.c_str());

    while (true)
    {
        if (menu->checkButtonEvents())
        {
            return;
        }
        delay(100);
    }
}

void UI::finishConfiguration(bool enableAutoReorder)
{
    preferences.setShouldReorderAutomatically(enableAutoReorder);

    tft.fillScreen(BACKGROUND_COLOR);
    auto bounds = typeTitle("Configuration complete");
    delay(2000);
    wipeText(bounds);
}

TextConfig UI::createTextConfig(const GFXfont *font)
{
    return TextConfig(font);
}

TextBounds UI::typeText(const char *text, const TextConfig &config)
{
    tft.setTextColor(config.textColor);
    tft.setFreeFont(config.font);
    tft.setTextSize(1);

    const int textLength = strlen(text);
    const int16_t cursorWidth = tft.textWidth("W"); // use a pretty wide block as the base width for our cursor

    int16_t fontHeight = 32; // Default in case we can't get actual height
    if (config.font && config.font->yAdvance > 0)
    {
        fontHeight = config.font->yAdvance;
    }

    // get min char width and space width
    const int16_t spaceWidth = tft.textWidth("a b") - tft.textWidth("ab");
    const int16_t cursorHeight = fontHeight;

    int16_t totalTextWidth = tft.textWidth(text);

    int16_t startX = config.x;
    int16_t textY = config.y;
    if (startX < 0)
    {
        startX = (tft.width() - totalTextWidth) / 2;
    }
    if (textY < 0)
    {
        textY = (tft.height() + fontHeight) / 2;
    }

    const int16_t cursorY = textY - cursorHeight + 6;
    int16_t cursorX = startX;
    const uint8_t cursorPadding = 4;

    TextBounds bounds;

    for (int i = 0; i < textLength; i++)
    {
        // Clean previous cursor
        if (i > 0 || config.enableCursor)
        {
            tft.fillRect(cursorX, cursorY, cursorWidth + cursorPadding, cursorHeight, BACKGROUND_COLOR);
        }

        int16_t singleCharWidth;

        if (text[i] == ' ')
        {
            // For spaces, just advance the cursor position without trying to render anything
            singleCharWidth = spaceWidth;
        }
        else
        {
            char currentChar[2] = {text[i], '\0'};
            singleCharWidth = tft.textWidth(currentChar);

            tft.setCursor(cursorX, textY);
            tft.print(currentChar);
        }

        cursorX += singleCharWidth;

        if (i < textLength - 1 || config.enableCursor)
        {
            tft.fillRect(cursorX + cursorPadding, cursorY, cursorWidth, cursorHeight, config.cursorColor);
        }

        bounds.x = startX;
        bounds.y = textY;
        bounds.width = totalTextWidth;
        bounds.height = fontHeight;
        bounds.cursorX = cursorX + cursorPadding;
        bounds.cursorY = cursorY;
        bounds.cursorWidth = cursorWidth;
        bounds.cursorHeight = cursorHeight;
        bounds.yAdvance = config.font ? config.font->yAdvance : 0;

        lastCursorState = bounds;

        delay(config.delay_ms);
    }

    return bounds;
}

TextBounds UI::typeTitle(const char *text, const TextConfig &config)
{
    auto cfg = config;
    cfg.font = getIdealFont(text, titleFonts);

    return typeText(text, cfg);
}

const GFXfont *UI::getIdealFont(const char *text, const std::vector<const GFXfont *> &fonts)
{
    const GFXfont *idealFont = fonts.back(); // default to smallest font

    for (const auto &font : fonts)
    {
        tft.setFreeFont(font);
        int16_t textWidth = tft.textWidth(text);

        if (textWidth < tft.width() - 16)
        {
            idealFont = font;
            break;
        }
    }

    return idealFont;
}

int16_t UI::setIdealFont(const char *text, uint16_t padding, const std::vector<const GFXfont *> &fonts)
{
    const GFXfont *idealFont = fonts.back(); // default to smallest font
    tft.setFreeFont(idealFont);
    int16_t textWidth = tft.textWidth(text);

    for (const auto &font : fonts)
    {
        tft.setFreeFont(font);
        int16_t fontTextWidth = tft.textWidth(text);

        if (fontTextWidth < tft.width() - padding)
        {
            Serial.printf("ideal font: has width %d which is < %d\n", fontTextWidth, tft.width() - padding);
            idealFont = font;
            textWidth = fontTextWidth;
            break;
        }
    }

    return textWidth;
}

int16_t UI::setIdealFont(const char *text, const std::vector<const GFXfont *> &fonts)
{
    return setIdealFont(text, 16, fonts);
}

TextBounds UI::typeBoundedText(const char *text, TextConfig config, std::vector<const GFXfont *> fonts)
{
    auto idealFont = getIdealFont(text, fonts);
    config.font = idealFont;
    return typeText(text, config);
}

void UI::wipeText(const TextBounds &bounds, int speed_ms)
{
    int16_t cursorX = bounds.cursorX;
    const int16_t wipeHeight = max(bounds.height, bounds.cursorHeight) + 8;

    // Calculate the full area that needs to be wiped
    int16_t areaStartX = bounds.x;
    int16_t areaWidth = bounds.width + bounds.cursorWidth + 20;

    // Smoothly sweep the cursor to the left
    while (cursorX > bounds.x)
    {
        tft.drawFastVLine(cursorX + bounds.cursorWidth, bounds.cursorY, wipeHeight, BACKGROUND_COLOR);
        cursorX--;
        tft.drawFastVLine(cursorX, bounds.cursorY, bounds.cursorHeight, ACCENT_COLOR);
        delay(speed_ms);
    }

    // Clear entire text area including cursor
    // Use a larger area than strictly necessary to ensure complete cleanup
    tft.fillRect(
        areaStartX - 5,
        bounds.cursorY - 5,
        areaWidth + 10,
        wipeHeight + 10,
        BACKGROUND_COLOR);
}

void UI::terminalAnimation()
{
    ledStrip.turnOnAnimation();
    tft.fillScreen(BACKGROUND_COLOR);

    const char *text = "terminal";
    auto config = createTextConfig(&GeistMono_VariableFont_wght18pt7b);
    TextBounds bounds = typeText(text, config);

    delay(1500);
    ledStrip.turnOffAnimation();
    delay(500);

    wipeText(bounds);

    tft.fillScreen(BACKGROUND_COLOR);
}

// Start blinking cursor using the last cursor position
void UI::startBlinking(unsigned long blinkInterval)
{
    if (lastCursorState.cursorWidth > 0 && lastCursorState.cursorHeight > 0)
    {
        startBlinkingAt(lastCursorState, blinkInterval);
    }
    else
    {
        Serial.println("Warning: No previous cursor position available for blinking");
    }
}

// Start blinking cursor at a specific position
void UI::startBlinkingAt(const TextBounds &bounds, unsigned long blinkInterval)
{
    stopBlinking();

    blinkState = new BlinkState;
    if (blinkState == NULL)
    {
        Serial.println("Error: Failed to allocate BlinkState");
        return;
    }

    blinkState->isVisible = true;
    blinkState->lastToggleTime = millis();
    blinkState->blinkInterval = blinkInterval;
    blinkState->bounds = bounds;
    blinkState->userData = this;

    BaseType_t result = xTaskCreatePinnedToCore(
        cursorBlinkTaskWrapper,
        "CursorBlink",
        4096,
        (void *)blinkState,
        8,
        &cursorBlinkTaskHandle,
        1 // needs to be pinned to core 1 because we are drawing on the display
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
        vTaskDelete(cursorBlinkTaskHandle);

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

        cursorBlinkTaskHandle = NULL;
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
    if (menu->current == STORE ||
        menu->current == STORE_ORDERS ||
        menu->current == STORE_BROWSE)
    {
        drawMenu();
        store->draw();
        return;
    }

    if (!scaleManager->bagRemovedFromSurface)
    {
        if (scaleManager->bagIsBelowThreshold)
        {
            if (menu->current == MAIN_MENU)
            {
                menu->selectMenu(MAIN_MENU_REORDER, false);
                menu->taint();
            }
        }
        else
        {
            if (menu->current == MAIN_MENU_REORDER)
            {
                menu->selectMenu(MAIN_MENU, false);
                menu->taint();
            }
        }

        if (scaleManager->bagIsBelowPromptThreshold)
        {
            if (preferences.shouldReorderAutomatically())
            {
                if (!preferences.doNotReorder())
                {
                    unsigned long startTime = millis();
                    unsigned long endTime = startTime + 60000;
                    long remaining = endTime - millis();

                    menu->selectMenu(MAIN_MENU_PROMPT_REORDER_AUTO, false);

                    int lastRenderedSecond = -1;
                    tft.fillScreen(ACCENT_COLOR);
                    ledStrip.reorderAnimation();

                    String text = "Press any button to cancel";
                    auto width = setIdealFont(text.c_str(), 16, nonTitleFonts);
                    tft.setCursor(tft.width() / 2 - width / 2, tft.height() / 2 + 40);
                    tft.setTextColor(BACKGROUND_COLOR);
                    tft.print(text.c_str());

                    while (remaining > 0)
                    {
                        remaining = endTime - millis();
                        auto second = (remaining / 1000);
                        text = "Reordering in " + String(second) + "s";

                        if (second != lastRenderedSecond)
                        {
                            lastRenderedSecond = second;
                            auto width = setIdealFont(text.c_str(), 16);
                            tft.fillRect(0, tft.height() / 2 - GeistMono_VariableFont_wght18pt7b.yAdvance / 2,
                                         tft.width(), GeistMono_VariableFont_wght18pt7b.yAdvance, ACCENT_COLOR);
                            tft.setCursor((tft.width() - width) / 2, tft.height() / 2);
                            tft.setTextColor(BACKGROUND_COLOR);
                            tft.print(text.c_str());
                        }

                        if (menu->checkButtonEvents())
                        {
                            preferences.setDoNotReorder(true);
                            menu->selectMenu(MAIN_MENU);
                            tft.fillScreen(BACKGROUND_COLOR);
                            ledStrip.turnOff();
                            taint();
                            return;
                        }

                        if (!scaleManager->bagIsBelowPromptThreshold)
                        {
                            menu->selectMenu(MAIN_MENU);
                            tft.fillScreen(BACKGROUND_COLOR);
                            ledStrip.turnOff();
                            taint();
                            return;
                        }

                        delay(500);
                        remaining = endTime - millis();
                    }

                    // store->orderProduct(scaleManager->bagName);
                    // preferences.setDoNotReorder(true);
                    tft.fillScreen(BACKGROUND_COLOR);
                    menu->selectMenu(MAIN_MENU);
                    ledStrip.turnOff();
                    taint();
                    return;
                }
                else
                {
                    ledStrip.reorderIndication();
                }
            }
            else if (!reorderPromptDismissed)
            {
                if (menu->current != MAIN_MENU_PROMPT_REORDER)
                {
                    ledStrip.reorderAnimation();
                    menu->selectMenu(MAIN_MENU_PROMPT_REORDER, false);
                    menu->taint();
                    drawMenu();
                    drawReorderPrompt();
                    return;
                }

                return;
            }
        }
        else
        {
            if (ledStrip.showingReorderIndicator)
            {
                ledStrip.turnOff();
                ledStrip.showingReorderIndicator = false;
            }
            if (menu->current == MAIN_MENU_PROMPT_REORDER)
            {
                menu->selectMenu(MAIN_MENU, false);
                menu->taint();
                ledStrip.turnOff();
            }
            reorderPromptDismissed = false;
        }
    }

    drawMenu();

    if (scaleManager->loadingBag)
    {
        if (reorderPromptDismissed)
        {
            reorderPromptDismissed = false;
        }
        bagSelect->draw();
        return;
    }

    if (scaleManager->bagRemovedFromSurface || !scaleManager->hasBag)
    {
        if (!drawnBagNotFound)
        {
            unsigned long currentTime = millis();
            auto textConfig = createTextConfig(&GeistMono_VariableFont_wght14pt7b);
            textConfig.y = tft.height() / 2 + titleText.font->yAdvance + 8;
            // FIXME: should probably be a background task?
            tft.fillRect(0, tft.height() / 2 - titleText.font->yAdvance - 8,
                         tft.width(), tft.height(), BACKGROUND_COLOR);

            auto titleTextConfig = titleText;
            titleTextConfig.enableCursor = false;

            auto bounds = typeText("404", titleTextConfig);
            bounds = typeText("Bag not found", textConfig);
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

    const uint16_t progressX = 20;
    const uint16_t progressHeight = 100;
    const uint16_t progressY = tft.height() - progressHeight - 20;
    const uint16_t progressWidth = 60;
    const int progressBarFill = constrain(progress * progressHeight, 0, progressHeight);

    auto text = String(weight, 1) + " g";

    // clear weight text area
    tft.fillRect(progressX + progressWidth + 10, progressY,
                 tft.width() - progressX - progressWidth - 20, progressHeight, BACKGROUND_COLOR);

    tft.setTextSize(1);
    tft.setFreeFont(&GeistMono_VariableFont_wght12pt7b);
    tft.setCursor(progressX + progressWidth + 10, progressY + GeistMono_VariableFont_wght12pt7b.yAdvance);
    tft.setTextColor(ACCENT_COLOR);
    tft.print(scaleManager->bagName.c_str());

    tft.setFreeFont(&GeistMono_VariableFont_wght18pt7b);
    tft.setTextColor(TEXT_COLOR);
    tft.setCursor(progressX + progressWidth + 10, progressY + progressHeight - 8);
    tft.print(text);

    if (progressBarFill == lastProgressBarFill)
    {
        return;
    }
    lastProgressBarFill = progressBarFill;

    tft.fillRect(progressX, progressY, progressWidth, progressHeight, BACKGROUND_COLOR);
    tft.drawRect(progressX, progressY, progressWidth, progressHeight, BAG_COLOR);
    tft.fillRect(progressX + 4, progressY + 4 + progressHeight - progressBarFill, progressWidth - 8, progressBarFill - 8, ACCENT_COLOR);

    for (int i = 1; i - 1 < progressBarFill / 20; i++)
    {
        tft.drawFastHLine(progressX + 2, progressY + 2 + progressHeight - i * 20, progressWidth - 4, BACKGROUND_COLOR);
    }
}

void UI::drawProgressIndicator(uint index, uint size)
{
    tft.setFreeFont(SMALL_FONT);
    const uint16_t y = tft.height() - 4;

    String text = String(index + 1) + "/" + String(size);
    const uint16_t x = tft.width() / 2 - tft.textWidth(text.c_str()) / 2;

    tft.setCursor(x, y);
    tft.setTextColor(MUTED_TEXT_COLOR);
    tft.print(text.c_str());
}

void UI::drawReorderPrompt()
{
    tft.fillRect(0, Menu::menuClearance, tft.width(), tft.height() - Menu::menuClearance, BACKGROUND_COLOR);

    auto bounds = typeTitle("Reorder bag?");
    startBlinkingAt(bounds);
}

void UI::dismissReorderPrompt()
{
    reorderPromptDismissed = true;
    tft.fillRect(0, Menu::menuClearance, tft.width(), tft.height() - Menu::menuClearance, BACKGROUND_COLOR);
    taint();
    stopBlinking();
    ledStrip.turnOff();
    menu->selectMenu(MAIN_MENU, false);
}