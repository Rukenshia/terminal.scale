#include "scale.h"
#include "buttons.h"
#include "ui.h"
#include "bag_select.h"

Scale::Scale(HX711 &scaleModule, TFT_eSPI &display, UI &uiSystem, PreferencesManager &prefs, TerminalApi &terminalApi, LedStrip &ledStrip, int dt_pin, int sck_pin)
    : scale(scaleModule),
      tft(display),
      ui(uiSystem),
      preferences(prefs),
      terminalApi(terminalApi),
      PIN_DT(dt_pin),
      PIN_SCK(sck_pin),
      calibrationRequested(false),
      ledStrip(ledStrip)
{
    calibrationFactor = 0.0;
    zeroOffset = 0;
    startBackgroundWeighingTask();
}

void Scale::begin()
{
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
        // not taring because the scale is expected to be constantly loaded and we want to
        // know the total weight placed on the scale, not relative to startup

        hasBag = preferences.hasCoffeeBag();
        if (hasBag)
        {
            bagName = preferences.getCoffeeBagName();
        }

        Serial.println("Scale calibrated and ready to use");
    }
    else
    {
        Serial.println("Scale not calibrated");
    }
}

void Scale::requestCalibration()
{
    calibrationRequested = true;
}

bool Scale::checkCalibrationRequest()
{
    if (calibrationRequested)
    {
        calibrationRequested = false;
        calibrate();
        return true;
    }
    return false;
}

void Scale::calibrate()
{
    stopBackgroundWeighingTask();

    tft.fillScreen(TFT_BLACK);

    TextConfig instructionConfig = ui.createTextConfig(MAIN_FONT);
    instructionConfig.x = 20;
    instructionConfig.y = 40;
    instructionConfig.enableCursor = false;
    instructionConfig.delay_ms = 20;

    auto bounds = ui.typeText("Calibration", titleText);
    delay(1000);

    // reset current values
    scale.set_scale();
    scale.set_offset(0);
    scale.tare();
    preferences.setHasCoffeeBag(false);

    ui.wipeText(bounds);

    instructionConfig.font = ui.getIdealFont("Place known weight", nonTitleFonts);
    ui.typeText("1. Place known weight", instructionConfig);

    instructionConfig.y = 80;
    instructionConfig.font = ui.getIdealFont("2. Press any button", nonTitleFonts);
    auto buttonBounds = ui.typeText("2. Press any button", instructionConfig);

    while (digitalRead(PIN_TOPLEFT) == HIGH &&
           digitalRead(PIN_TOPMIDDLE) == HIGH &&
           digitalRead(PIN_TOPRIGHT) == HIGH &&
           digitalRead(PIN_TERMINAL_BUTTON) == HIGH)
    {
        delay(100);
    }

    delay(200);

    tft.fillScreen(TFT_BLACK);

    instructionConfig.y = 80;
    instructionConfig.font = ui.getIdealFont("Measuring...", nonTitleFonts);
    auto measuringBounds = ui.typeText("Measuring...", instructionConfig);

    delay(2000);

    long reading = scale.get_units(10);
    long offset = scale.get_offset();

    ui.wipeText(measuringBounds);

    char buf[32];
    snprintf(buf, sizeof(buf), "Reading: %ld", reading);
    instructionConfig.y = 80;
    auto readingBounds = ui.typeText(buf, instructionConfig);

    instructionConfig.y = 120;
    instructionConfig.font = ui.getIdealFont("Enter weight in mg", nonTitleFonts);
    ui.typeText("Enter weight in mg", instructionConfig);
    instructionConfig.y = 160;
    instructionConfig.font = ui.getIdealFont("via Serial Monitor", nonTitleFonts);
    ui.typeText("via Serial Monitor", instructionConfig);

    while (Serial.available() == 0)
    {
        delay(100);
    }

    String input = Serial.readStringUntil('\n');
    Serial.printf("received: '%s'\n", input.c_str());
    long knownWeightMg = input.toInt();
    Serial.printf("Known weight: %ld\n", knownWeightMg);

    calibrationFactor = reading * 1000.0f / knownWeightMg;
    zeroOffset = offset;

    preferences.setScaleCalibrationFactor(calibrationFactor);
    preferences.setScaleZeroOffset(zeroOffset);

    tft.fillScreen(TFT_BLACK);
    instructionConfig.y = 40;
    bounds = ui.typeText("Calibrated", titleText);

    char factorBuf[32];
    snprintf(factorBuf, sizeof(factorBuf), "cf=%.2f, zo=%ld", calibrationFactor, zeroOffset);
    instructionConfig.y = 80;
    ui.typeText(factorBuf, instructionConfig);

    instructionConfig.y = 120;
    instructionConfig.font = ui.getIdealFont("Restarting...", nonTitleFonts);
    auto restartBounds = ui.typeText("Restarting...", instructionConfig);

    esp_restart();
}

void Scale::startLoadBag()
{
    loadingBag = true;

    tft.fillScreen(BACKGROUND_COLOR);
    auto bounds = ui.typeText("Loading...");
    ui.startBlinking();

    auto products = terminalApi.getProducts();
    std::vector<String> bagList;
    bagList.reserve(products.size());

    std::transform(
        products.begin(),
        products.end(),
        std::back_inserter(bagList),
        [](const Product &product)
        { return product.name; });
    bagList.push_back("[object Object]");
    bagList.push_back("segmentation fault");
    ui.bagSelect->setBags(bagList);

    ui.stopBlinking();
    ui.wipeText(bounds);

    ui.menu->selectMenu(SELECT_BAG);
    ui.bagSelect->taint();
}

void Scale::loadBag(String name)
{
    Serial.println("Loading bag");

    stopBackgroundWeighingTask();

    tft.fillScreen(BACKGROUND_COLOR);

    TextConfig instructionConfig = ui.createTextConfig(&GeistMono_VariableFont_wght14pt7b);
    instructionConfig.y = tft.height() / 2 - 20;
    instructionConfig.enableCursor = false;
    instructionConfig.delay_ms = 20;

    ui.typeText("Place 12oz bag", instructionConfig);

    instructionConfig.y += instructionConfig.font->yAdvance + 12;
    instructionConfig.font = ui.getIdealFont("and press any button", nonTitleFonts);
    ui.typeText("and press any button", instructionConfig);

    while (digitalRead(PIN_TOPLEFT) == HIGH &&
           digitalRead(PIN_TOPMIDDLE) == HIGH &&
           digitalRead(PIN_TOPRIGHT) == HIGH &&
           digitalRead(PIN_TERMINAL_BUTTON) == HIGH)
    {
        delay(100);
    }

    tft.fillScreen(BACKGROUND_COLOR);
    instructionConfig.font = &GeistMono_VariableFont_wght12pt7b;
    instructionConfig.y = tft.height() / 2;
    auto bounds = ui.typeText("Measuring...", instructionConfig);

    float reading = scale.get_units(10);
    weightBeforeLoadBag = reading;

    ui.wipeText(bounds);

    instructionConfig.font = &GeistMono_VariableFont_wght16pt7b;
    bounds = ui.typeText((String(reading, 1) + " g").c_str(), instructionConfig);

    instructionConfig.y += instructionConfig.font->yAdvance + 8;
    instructionConfig.textColor = ACCENT_COLOR;

    String text = String("-") + String(TERMINAL_COFFEE_BAG_EMPTY_WEIGHT, 2) + " g (bag)";
    instructionConfig.font = ui.getIdealFont(text.c_str(), nonTitleFonts);

    ui.typeText(text.c_str(), instructionConfig);

    ui.menu->clearButtons();
    ui.menu->selectMenu(LOADING_BAG_CONFIRM);

    bagName = name;
    startBackgroundWeighingTask();

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
    preferences.setCoffeeBagName(bagName);
    preferences.setDoNotReorder(false);
    hasBag = true;
    loadingBag = false;
    bagName = "flow";

    ui.taint();
    ui.menu->clearButtons();
    ui.menu->selectMenu(MAIN_MENU);
}

float Scale::readWeight(int samples)
{
    if (scale.wait_ready_retry(3, 50))
    {
        float reading = scale.get_units(samples);

        if (hasBag && !baristaMode)
        {
            reading -= TERMINAL_COFFEE_BAG_EMPTY_WEIGHT;
        }

        lastReading = reading;
        return reading;
    }
    return -1;
}

void Scale::tare()
{
    if (backgroundWeighingTaskHandle != NULL)
    {
        vTaskSuspend(backgroundWeighingTaskHandle);
    }

    if (scale.wait_ready_retry(3, 50))
    {
        scale.tare();
    }

    if (backgroundWeighingTaskHandle != NULL)
    {
        vTaskResume(backgroundWeighingTaskHandle);
    }
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

    float minReading = 0.0f;
    float maxReading = 0.0f;
    float lastReading = 0.0f;

    while (true)
    {
        float reading = scale->readWeight();
        reading = round(reading * 10.0) / 10.0;

        minReading = min(minReading, reading);
        maxReading = max(maxReading, reading);

        if (reading != lastReading)
        {
            Serial.printf("hasBag=%d, reading=%.1f min=%.1f max=%.1f\n", scale->hasBag, reading, minReading, maxReading);
        }

        lastReading = reading;

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

        if (scale->hasBag && !scale->bagRemovedFromSurface)
        {
            if (reading < REORDER_BUTTON_THRESHOLD)
            {
                scale->bagIsBelowThreshold = true;
            }
            else
            {
                scale->bagIsBelowThreshold = false;
            }

            if (reading < REORDER_BUTTON_PROMPT_THRESHOLD)
            {
                scale->bagIsBelowPromptThreshold = true;
            }
            else
            {
                scale->bagIsBelowPromptThreshold = false;
            }
        }

        vTaskDelay(scale->backgroundWeighingDelay / portTICK_PERIOD_MS);
    }
}

void Scale::startBackgroundWeighingTask()
{
    if (backgroundWeighingTaskHandle != NULL)
    {
        return;
    }

    xTaskCreate(
        backgroundWeighingTask,
        "BackgroundWeighing",
        4096,
        this,
        4,
        &backgroundWeighingTaskHandle);
}

void Scale::stopBackgroundWeighingTask()
{
    if (backgroundWeighingTaskHandle != NULL)
    {
        vTaskDelete(backgroundWeighingTaskHandle);
        backgroundWeighingTaskHandle = NULL;
    }
}

// Enter Barista mode: single shot by default
void Scale::enterBaristaMode()
{
    tft.fillScreen(BACKGROUND_COLOR);
    baristaMode = true;
    ui.menu->selectMenu(BARISTA_SINGLE);
    ui.taint();
    backgroundWeighingDelay = 100;
    tare();
}

// Exit Barista mode: return to main menu
void Scale::leaveBaristaMode()
{
    scale.set_offset(preferences.getScaleZeroOffset());
    backgroundWeighingDelay = 1000;
    baristaMode = false;
    baristaLastProgress = -99;
    baristaLastDrawnReading = -99.0f;
    ui.menu->selectMenu(MAIN_MENU);
    ledStrip.turnOff();
    ui.taint();
}

// Draw Barista mode UI with progress towards target shot weight
void Scale::drawBaristaMode()
{
    // get current weight reading
    float weight = lastReading;
    weight = round(weight * 10.0f) / 10.0f;
    if (weight == -0.0f)
    {
        weight = 0.0f;
    }

    if (weight == baristaLastDrawnReading)
    {
        return;
    }
    baristaLastDrawnReading = weight;

    // determine target based on mode
    float target = (ui.menu->current == BARISTA_SINGLE) ? 12.0f : 21.0f;

    // calculate progress 0.0 to 1.0
    float progress = weight / target;
    float constraintedProgress = constrain(progress, 0.0f, 1.0f);

    auto textColor = TEXT_COLOR;

    auto diff = abs(target - weight);
    if (diff < 0.6f)
    {
        textColor = TEXT_COLOR_GREEN;
        ledStrip.setColor(RgbColor(0, 32, 0));
    }
    else if (progress > 1.0f)
    {
        textColor = TEXT_COLOR_RED;
        ledStrip.setColor(RgbColor(32, 0, 0));
    }
    else
    {
        ledStrip.progress(constraintedProgress, RgbColor(255 / 5, 94 / 5, 0));
    }

    progress = constraintedProgress;

    const uint16_t progressX = 20;
    const uint16_t progressHeight = 100;
    const uint16_t progressY = tft.height() - progressHeight - 20;
    const uint16_t progressWidth = 60;
    int progressBarFill = constrain((int)(progress * progressHeight), 0, progressHeight);

    // clear text area
    tft.fillRect(progressX + progressWidth + 10, progressY,
                 tft.width(), progressHeight, BACKGROUND_COLOR);

    // show mode label
    tft.setTextSize(1);
    tft.setFreeFont(&GeistMono_VariableFont_wght12pt7b);
    tft.setCursor(progressX + progressWidth + 10, progressY + GeistMono_VariableFont_wght12pt7b.yAdvance);
    tft.setTextColor(TEXT_COLOR);
    tft.print((ui.menu->current == BARISTA_SINGLE) ? "Single" : "Double");

    // show weight vs target
    tft.setFreeFont(&GeistMono_VariableFont_wght16pt7b);

    String text = String(weight, 1) + "g";
    auto textWidth = tft.textWidth(text.c_str());
    tft.setCursor(progressX + progressWidth + 10, progressY + progressHeight - 8);
    tft.setTextColor(textColor);
    tft.print(text.c_str());

    tft.setFreeFont(&GeistMono_VariableFont_wght12pt7b);
    text = "/ " + String(target, 1) + "g";
    tft.setCursor(progressX + progressWidth + 10 + textWidth + 8, progressY + progressHeight - 8);
    tft.print(text.c_str());

    if (progressBarFill == baristaLastProgress)
    {
        return;
    }
    baristaLastProgress = progressBarFill;

    // draw progress bar
    tft.fillRect(progressX, progressY, progressWidth, progressHeight, BACKGROUND_COLOR);
    tft.drawRect(progressX, progressY, progressWidth, progressHeight, BAG_COLOR);
    tft.fillRect(progressX + 4,
                 progressY + 4 + progressHeight - progressBarFill,
                 progressWidth - 8,
                 progressBarFill - 8,
                 textColor);
}

void Scale::forceBaristaRedraw()
{
    baristaLastProgress = -99;
    baristaLastDrawnReading = -99.0f;
}