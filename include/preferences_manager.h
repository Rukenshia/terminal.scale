#ifndef PREFERENCES_MANAGER_H
#define PREFERENCES_MANAGER_H

#include <Arduino.h>
#include <Preferences.h> // Using angle brackets for Arduino ESP32 library

class PreferencesManager
{
private:
    Preferences preferences; // Using private member with underscore prefix

public:
    PreferencesManager();
    void begin();
    void end();

    bool isScaleCalibrated();
    void setScaleCalibrationFactor(float calibrationFactor);
    void setScaleZeroFactor(float zeroFactor);

    float getScaleCalibrationFactor();
    float getScaleZeroFactor();
};

#endif