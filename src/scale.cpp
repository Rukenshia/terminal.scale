#include "scale.h"
#include "buttons.h"

// Constructor
Scale::Scale(HX711 &scaleModule, TFT_eSPI &display, UI &uiSystem, PreferencesManager &prefs, int dt_pin, int sck_pin)
    : scale(scaleModule),
      tft(display),
      ui(uiSystem),
      preferences(prefs),
      PIN_DT(dt_pin),
      PIN_SCK(sck_pin),
      calibrationRequested(false)
{
    calibrationFactor = 0.0;
    zeroOffset = 0;
}

void Scale::begin()
{
    // Initialize the scale
    scale.begin(PIN_DT, PIN_SCK);

    // If calibration data exists, load it
    if (preferences.isScaleCalibrated())
    {
        calibrationFactor = preferences.getScaleCalibrationFactor();
        zeroOffset = preferences.getScaleZeroOffset();

        Serial.printf("Using saved calibration factor: %.2f\n", calibrationFactor);
        Serial.printf("Using saved zero offset: %ld\n", zeroOffset);

        scale.set_scale(calibrationFactor);
        scale.set_offset(zeroOffset);
        scale.tare();

        Serial.println("Scale calibrated and ready to use");
    }
    else
    {
        Serial.println("Scale not calibrated");
    }
}

// Safe to call from any context including interrupts
void Scale::requestCalibration()
{
    // Set the flag to request calibration
    calibrationRequested = true;
}

// Check and handle pending calibration request - call from main loop
bool Scale::checkCalibrationRequest()
{
    if (calibrationRequested)
    {
        calibrationRequested = false; // Reset the flag
        calibrate();                  // Do the actual calibration
        return true;
    }
    return false;
}

void Scale::calibrate()
{
    // Safe to call from anywhere - encapsulates all UI and scale operations
    scale.set_scale(); // Set the scale to the default calibration factor
    scale.tare();      // Reset the scale to 0

    // Clear the screen and show initial instructions
    tft.fillScreen(TFT_BLACK);

    TextConfig instructionConfig = ui.createTextConfig(MAIN_FONT);
    instructionConfig.y = 40;
    instructionConfig.enableCursor = false;

    auto bounds = ui.typeText("Calibration", titleText);
    delay(1000);
    ui.wipeText(bounds);

    ui.typeText("1. Place a known weight", instructionConfig);

    instructionConfig.y = 80;
    auto buttonBounds = ui.typeText("2. Press any button", instructionConfig);

    // Wait for button press
    while (digitalRead(PIN_TOPLEFT) == HIGH &&
           digitalRead(PIN_TOPMIDDLE) == HIGH &&
           digitalRead(PIN_TOPRIGHT) == HIGH &&
           digitalRead(PIN_TERMINAL_BUTTON) == HIGH)
    {
        delay(100);
    }

    // Debounce
    delay(200);

    tft.fillScreen(TFT_BLACK);

    instructionConfig.y = 80;
    auto measuringBounds = ui.typeText("Measuring...", instructionConfig);

    // Take multiple readings for better accuracy
    long reading = scale.get_units(10);

    ui.wipeText(measuringBounds);

    // Show the reading and prompt for weight input
    char buf[32];
    snprintf(buf, sizeof(buf), "Reading: %ld", reading);
    instructionConfig.y = 80;
    auto readingBounds = ui.typeText(buf, instructionConfig);

    instructionConfig.y = 120;
    ui.typeText("Enter weight in grams", instructionConfig);
    instructionConfig.y = 160;
    ui.typeText("via Serial Monitor", instructionConfig);

    // Wait for serial input
    while (Serial.available() == 0)
    {
        delay(100);
    }

    float knownWeight = Serial.parseFloat();

    // Avoid division by zero
    if (knownWeight == 0)
    {
        knownWeight = 1.0;
        Serial.println("Warning: Zero weight entered, using 1g as default");
    }

    calibrationFactor = reading / knownWeight;
    zeroOffset = scale.get_offset();

    // Save calibration factor to preferences
    preferences.setScaleCalibrationFactor(calibrationFactor);
    preferences.setScaleZeroOffset(zeroOffset);

    // Show completion message
    tft.fillScreen(TFT_BLACK);
    instructionConfig.y = 40;
    bounds = ui.typeText("Calibrated", titleText);

    char factorBuf[32];
    snprintf(factorBuf, sizeof(factorBuf), "cf=%.2f, zo=%ld", calibrationFactor, zeroOffset);
    instructionConfig.y = 80;
    ui.typeText(factorBuf, instructionConfig);

    instructionConfig.y = 120;
    auto restartBounds = ui.typeText("Restarting...", instructionConfig);

    // Apply the calibration factor
    scale.set_scale(calibrationFactor);
    scale.tare(); // Reset the scale to 0

    // Wait a moment before returning
    delay(2000);

    // Clear screen when finished
    tft.fillScreen(TFT_BLACK);
}

float Scale::readWeight(int samples)
{
    if (scale.wait_ready_timeout(200))
    {
        return scale.get_units(samples);
    }
    return -1; // Error value
}

void Scale::tare()
{
    scale.tare();
}

bool Scale::isCalibrated()
{
    return preferences.isScaleCalibrated();
}

float Scale::getCalibrationFactor()
{
    return calibrationFactor;
}

long Scale::getZeroOffset()
{
    return zeroOffset;
}