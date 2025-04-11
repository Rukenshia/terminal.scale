#ifndef SCALE_H
#define SCALE_H

#include <Arduino.h>
#include <HX711.h>
#include <TFT_eSPI.h>
#include "ui.h"
#include "preferences_manager.h"

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

public:
    Scale(HX711 &scaleModule, TFT_eSPI &display, UI &uiSystem, PreferencesManager &prefs, int dt_pin, int sck_pin);

    // Initialize the scale
    void begin();

    // Calibrate the scale (should only be called from main loop, not interrupt context)
    void calibrate();

    // Request calibration (safe to call from any context including interrupts)
    void requestCalibration();

    // Check and handle pending calibration request (call this from main loop)
    bool checkCalibrationRequest();

    // Read weight from scale
    float readWeight(int samples = 20);

    // Tare the scale (set to zero)
    void tare();

    // Check if the scale is calibrated
    bool isCalibrated();

    // Get calibration factor
    float getCalibrationFactor();

    // Get zero offset
    long getZeroOffset();
};

#endif