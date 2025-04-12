#ifndef SCALE_H
#define SCALE_H

#include <Arduino.h>
#include <HX711.h>
#include <TFT_eSPI.h>

class UI;
#include "preferences_manager.h"

#define TERMINAL_COFFEE_BAG_EMPTY_WEIGHT 16.0f
#define TERMINAL_COFFEE_WEIGHT 340.0f // 12oz
#define TERMINAL_COFFEE_BAG_WEIGHT TERMINAL_COFFEE_WEIGHT + TERMINAL_COFFEE_BAG_EMPTY_WEIGHT

class Scale
{
private:
    HX711 &scale;
    TFT_eSPI &tft;
    UI &ui;
    PreferencesManager &preferences;

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

    TaskHandle_t backgroundWeighingTaskHandle = NULL;

public:
    Scale(HX711 &scaleModule, TFT_eSPI &display, UI &uiSystem, PreferencesManager &prefs, int dt_pin, int sck_pin);

    bool hasBag = false;
    float lastReading = 0.0f;

    bool bagRemovedFromSurface = false;
    unsigned long bagRemovedTime = 0;
    

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

    void loadBag();
    void confirmLoadBag();

    static void backgroundWeighingTask(void *parameter);
};

#endif