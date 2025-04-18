#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Adafruit_GFX.h>
#include <vector>
#include "image_loader.h"
#include "menu.h"
#include "led.h"
#include "terminal_api.h"

class Scale;

#include "GeistMonoVariableFont_wght18.h"
#include "GeistMonoVariableFont_wght16.h"
#include "GeistMonoVariableFont_wght14.h"
#include "GeistMonoVariableFont_wght12.h"
#include "GeistMonoVariableFont_wght10.h"

#define ACCENT_COLOR 0x02ff
#define PRIMARY_COLOR TFT_WHITE
#define BACKGROUND_COLOR TFT_BLACK
#define TEXT_COLOR 0xBDD8
#define BAG_COLOR 0x736C

#define MAIN_FONT &GeistMono_VariableFont_wght10pt7b

const std::vector<const GFXfont *> nonTitleFonts = {
    &GeistMono_VariableFont_wght12pt7b,
    &GeistMono_VariableFont_wght10pt7b};

// Text bounds structure to store position and dimensions of typed text
struct TextBounds
{
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
    int16_t cursorX;
    int16_t cursorY;
    int16_t cursorWidth;
    int16_t cursorHeight;
    int16_t yAdvance;
};

// Configuration for text animation
struct TextConfig
{
    const GFXfont *font;  // Font to use
    int delay_ms;         // Delay between characters
    int16_t x;            // X position (-1 for auto-center)
    int16_t y;            // Y position (-1 for auto-center)
    uint16_t textColor;   // Text color
    uint16_t cursorColor; // Cursor color
    bool enableCursor;    // Whether to show cursor during typing

    // Constructor with default values
    TextConfig(const GFXfont *font = nullptr) : font(font),
                                                delay_ms(100),
                                                x(-1),
                                                y(-1),
                                                textColor(PRIMARY_COLOR),
                                                cursorColor(ACCENT_COLOR),
                                                enableCursor(true) {}

    TextConfig(const GFXfont *font, int delay_ms, int16_t x, int16_t y,
               uint16_t textColor, uint16_t cursorColor, int16_t cursorWidth, bool enableCursor)
        : font(font),
          delay_ms(delay_ms),
          x(x),
          y(y),
          textColor(textColor),
          cursorColor(cursorColor),
          enableCursor(enableCursor) {}
};

static const TextConfig defaultText = TextConfig(&GeistMono_VariableFont_wght14pt7b, 100, -1, -1, TEXT_COLOR, ACCENT_COLOR, 20, true);
static const TextConfig accentText = TextConfig(&GeistMono_VariableFont_wght14pt7b, 100, -1, -1, ACCENT_COLOR, ACCENT_COLOR, 20, true);
static const TextConfig titleText = TextConfig(&GeistMono_VariableFont_wght18pt7b, 100, -1, -1, TFT_WHITE, ACCENT_COLOR, 20, true);
static const TextConfig smallTitleText = TextConfig(&GeistMono_VariableFont_wght16pt7b, 100, -1, -1, TFT_WHITE, ACCENT_COLOR, 20, true);

// Structure to store blinking cursor state
struct BlinkState
{
    bool isVisible;
    unsigned long lastToggleTime;
    unsigned long blinkInterval;
    TextBounds bounds;
    void *userData; // Pointer to UI instance
};

// Forward declaration of the FreeRTOS task function
extern "C" void
cursorBlinkTaskWrapper(void *parameter);

class Menu;
class BagSelect;
class Store;

class UI
{
public:
    UI(TFT_eSPI &tftDisplay, LedStrip &ledStrip, TerminalApi &terminalApi);

    Menu *menu;
    BagSelect *bagSelect;
    Store *store;

    // Initialize the UI
    void begin(Scale *scaleManager);

    // Create a default text configuration
    TextConfig createTextConfig(const GFXfont *font = nullptr);

    // Typing animation that displays text letter by letter using configuration
    TextBounds typeText(const char *text, const TextConfig &config = defaultText);
    TextBounds typeBoundedText(const char *text, TextConfig config = defaultText, std::vector<const GFXfont *> fonts = {&GeistMono_VariableFont_wght18pt7b, &GeistMono_VariableFont_wght16pt7b, &GeistMono_VariableFont_wght14pt7b, &GeistMono_VariableFont_wght12pt7b, &GeistMono_VariableFont_wght10pt7b});
    const GFXfont *getIdealFont(const char *text, const std::vector<const GFXfont *> &fonts = {&GeistMono_VariableFont_wght18pt7b, &GeistMono_VariableFont_wght16pt7b, &GeistMono_VariableFont_wght14pt7b, &GeistMono_VariableFont_wght12pt7b, &GeistMono_VariableFont_wght10pt7b});
    // Get ideal font that doesn't clip the tft, returns width
    int16_t setIdealFont(const char *text, const std::vector<const GFXfont *> &fonts = {&GeistMono_VariableFont_wght18pt7b, &GeistMono_VariableFont_wght16pt7b, &GeistMono_VariableFont_wght14pt7b, &GeistMono_VariableFont_wght12pt7b, &GeistMono_VariableFont_wght10pt7b});
    int16_t setIdealFont(const char *text, uint16_t padding, const std::vector<const GFXfont *> &fonts = {&GeistMono_VariableFont_wght18pt7b, &GeistMono_VariableFont_wght16pt7b, &GeistMono_VariableFont_wght14pt7b, &GeistMono_VariableFont_wght12pt7b, &GeistMono_VariableFont_wght10pt7b});

    // Wipe animation for text that was displayed with typeText
    void wipeText(const TextBounds &bounds, int speed_ms = 1);

    // Animation for terminal
    void terminalAnimation();

    void startBlinking(unsigned long blinkInterval = 500);
    void startBlinkingAt(const TextBounds &bounds, unsigned long blinkInterval = 500);
    void stopBlinking();
    bool isBlinking();

    void drawMenu();

    void drawWeight(float weight);

    void loop();

private:
    TFT_eSPI &tft;
    TerminalApi &terminalApi;
    LedStrip &ledStrip;
    ImageLoader imageLoader;
    TaskHandle_t cursorBlinkTaskHandle;
    BlinkState *blinkState;
    TextBounds lastCursorState;
    Scale *scaleManager;

    int lastProgressBarFill = 0;
    float lastDrawnReading = 0.0f;

    bool drawnBagNotFound = false;

    friend void cursorBlinkTaskWrapper(void *parameter);
};

#endif