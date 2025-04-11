#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <Arduino.h>
#include <Preferences.h>

class PreferencesManager
{
private:
    Preferences preferences;

public:
    PreferencesManager();
    void begin();
    void end();

    bool isScaleCalibrated();
    void setScaleCalibrated(bool calibrated);
};

#endif