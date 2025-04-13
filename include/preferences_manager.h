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
    void begin(bool readOnly = false);
    void end();

    bool isScaleCalibrated();
    void setScaleCalibrationFactor(float calibrationFactor);
    void setScaleZeroOffset(long zeroOffset);

    float getScaleCalibrationFactor();
    long getScaleZeroOffset();

    bool hasCoffeeBag();
    void setHasCoffeeBag(bool hasCoffeeBag);

    String getCoffeeBagName();
    void setCoffeeBagName(const String &name);
};

#endif