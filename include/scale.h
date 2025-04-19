#ifndef SCALE_H
#define SCALE_H

#include <Arduino.h>
#include <HX711.h>
#include <TFT_eSPI.h>

class UI;
#include "preferences_manager.h"
#include "terminal_api.h"
#include "led.h"

#define TERMINAL_COFFEE_BAG_EMPTY_WEIGHT 15.2f
#define TERMINAL_COFFEE_WEIGHT 340.0f // 12oz
#define TERMINAL_COFFEE_BAG_WEIGHT TERMINAL_COFFEE_WEIGHT + TERMINAL_COFFEE_BAG_EMPTY_WEIGHT
#define REORDER_BUTTON_THRESHOLD 150.0f
#define REORDER_BUTTON_PROMPT_THRESHOLD 80.0f

#define SINGLE_DOSE_WEIGHT 8.0f
#define DOUBLE_DOSE_WEIGHT 16.0f

#define TEXT_COLOR_RED 0xD165
#define TEXT_COLOR_GREEN 0x6E24

class Scale
{
private:
    HX711 &scale;
    TFT_eSPI &tft;
    UI &ui;
    PreferencesManager &preferences;
    TerminalApi &terminalApi;
    LedStrip &ledStrip;

    // Pin configuration
    const int PIN_DT;
    const int PIN_SCK;

    // Calibration values
    float calibrationFactor;
    long zeroOffset;

    // Flag for safely requesting calibration from any context
    volatile bool calibrationRequested;

    // Weight measured before confirming load bag
    float weightBeforeLoadBag = 0.0f;

    float baristaLastDrawnReading = -99.0f;
    int baristaLastProgress = -99;
    bool baristaMode = false;

    TaskHandle_t backgroundWeighingTaskHandle = NULL;

public:
    Scale(HX711 &scaleModule, TFT_eSPI &display, UI &uiSystem, PreferencesManager &prefs, TerminalApi &terminalApi, LedStrip &ledStrip, int dt_pin, int sck_pin);

    bool hasBag = false;
    bool loadingBag = false;
    String bagName = "Unknown";
    volatile float lastReading = 0.0f;

    volatile bool fastMeasuring = false;

    bool bagRemovedFromSurface = false;
    unsigned long bagRemovedTime = 0;

    bool bagIsBelowThreshold = false;
    bool bagIsBelowPromptThreshold = false;

    // Initialize the scale
    void begin();

    // Calibrate the scale (should only be called from main loop, not interrupt context)
    void calibrate();

    // Request calibration (safe to call from any context including interrupts)
    void requestCalibration();

    // Check and handle pending calibration request (call this from main loop)
    bool checkCalibrationRequest();

    // Read weight from scale
    float readWeight(int samples = 10);

    // Tare the scale (set to zero)
    void tare();

    // Check if the scale is calibrated
    bool isCalibrated();

    // Get calibration factor
    float getCalibrationFactor();

    // Get zero offset
    long getZeroOffset();

    void startLoadBag();
    void loadBag(String name);
    void confirmLoadBag();

    // Enter and leave Barista mode
    void enterBaristaMode();
    void leaveBaristaMode();
    // Draw UI for Barista mode
    void drawBaristaMode();
    void forceBaristaRedraw();

    static void backgroundWeighingTask(void *parameter);
    void stopBackgroundWeighingTask();
    void startBackgroundWeighingTask();
};

#endif