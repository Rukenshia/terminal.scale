#include "scale.h"
#include "buttons.h"
#include "ui.h"

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

    xTaskCreate(
        backgroundWeighingTask, // Function that implements the task
        "BackgroundWeighing",   // Text name for the task
        4096,                   // Stack size in words
        this,                   // Parameter passed into the task
        4,                      // Priority of the task (1 is low)
        &backgroundWeighingTaskHandle);
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
        // scale.tare();

        hasBag = preferences.hasCoffeeBag();

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
    preferences.setHasCoffeeBag(false);

    // Clear the screen and show initial instructions
    tft.fillScreen(TFT_BLACK);

    TextConfig instructionConfig = ui.createTextConfig(MAIN_FONT);
    instructionConfig.x = 20;
    instructionConfig.y = 40;
    instructionConfig.enableCursor = false;
    instructionConfig.delay_ms = 20;

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

    esp_restart();
}

void Scale::loadBag()
{
    tft.fillScreen(BACKGROUND_COLOR);

    TextConfig instructionConfig = ui.createTextConfig(&GeistMono_VariableFont_wght12pt7b);
    instructionConfig.y = tft.height() / 2 - 20;
    instructionConfig.enableCursor = false;
    instructionConfig.delay_ms = 20;

    ui.typeText("Place 12oz bag", instructionConfig);

    instructionConfig.y += instructionConfig.font->yAdvance + 12;
    instructionConfig.font = &GeistMono_VariableFont_wght10pt7b;
    ui.typeText("and press any button", instructionConfig);
    instructionConfig.font = &GeistMono_VariableFont_wght12pt7b;

    // Wait for button press
    while (digitalRead(PIN_TOPLEFT) == HIGH &&
           digitalRead(PIN_TOPMIDDLE) == HIGH &&
           digitalRead(PIN_TOPRIGHT) == HIGH &&
           digitalRead(PIN_TERMINAL_BUTTON) == HIGH)
    {
        delay(100);
    }

    tft.fillScreen(BACKGROUND_COLOR);
    instructionConfig.y = tft.height() / 2;
    auto bounds = ui.typeText("Measuring...", instructionConfig);

    // get reading
    float reading = scale.get_units(20);
    weightBeforeLoadBag = reading;

    ui.wipeText(bounds);

    bounds = ui.typeText((String(reading, 1) + " g").c_str(), instructionConfig);

    instructionConfig.y += instructionConfig.font->yAdvance + 8;
    instructionConfig.textColor = ACCENT_COLOR;
    instructionConfig.font = &GeistMono_VariableFont_wght10pt7b;

    ui.typeText((String("-") + String(TERMINAL_COFFEE_BAG_EMPTY_WEIGHT, 2) + " g (bag)").c_str(), instructionConfig);

    ui.menu->clearButtons();
    ui.menu->selectMenu(LOADING_BAG_CONFIRM);

    // Wait for button press
    while (true)
    {
        if (ui.menu->checkButtonEvents())
        {
            break;
        }
        delay(100);
    }
}

void Scale::confirmLoadBag()
{
    tft.fillScreen(BACKGROUND_COLOR);

    auto bounds = ui.typeText("Bag loaded", titleText);
    delay(1000);
    ui.wipeText(bounds);

    preferences.setHasCoffeeBag(true);
    hasBag = true;

    ui.menu->clearButtons();
    ui.menu->selectMenu(MAIN_MENU);
}

float Scale::readWeight(int samples)
{
    if (scale.wait_ready_timeout(200))
    {
        float reading = scale.get_units(samples);
        lastReading = reading;

        if (hasBag)
        {
            lastReading = reading - TERMINAL_COFFEE_BAG_EMPTY_WEIGHT;
            return reading - TERMINAL_COFFEE_BAG_EMPTY_WEIGHT;
        }

        return reading;
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

void Scale::backgroundWeighingTask(void *parameter)
{
    Scale *scale = static_cast<Scale *>(parameter);

    while (true)
    {
        // Read weight and update the display
        float reading = scale->readWeight();
        reading = round(reading * 10.0) / 10.0;

        Serial.printf("hasBag=%d, reading=%.1f\n", scale->hasBag, reading);

        if (scale->hasBag && reading < 0)
        {
            // Bag was removed from the plate. Start a timer (2 minutes) to wait for the bag to be put back
            // If the timer expires, we need to jump over to the re-ordering screen

            scale->bagRemovedFromSurface = true;
            scale->bagRemovedTime = millis();
        }
        else if (scale->hasBag && scale->bagRemovedFromSurface)
        {
            // Bag was put back on the plate
            scale->bagRemovedFromSurface = false;
            scale->bagRemovedTime = 0;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}