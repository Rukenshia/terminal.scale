#include "preferences_manager.h"

PreferencesManager::PreferencesManager()
{
}

void PreferencesManager::begin(bool readOnly)
{
    preferences.begin("scale", readOnly);
}

void PreferencesManager::end()
{
    preferences.end();
}

void PreferencesManager::setScaleCalibrationFactor(float factor)
{
    begin();
    preferences.putFloat("cf", factor);
    end();
}

void PreferencesManager::setScaleZeroOffset(long zeroOffset)
{
    begin(true);
    preferences.putFloat("zo", zeroOffset);
    end();
}

float PreferencesManager::getScaleCalibrationFactor()
{
    begin(true);
    auto v = preferences.getFloat("cf", 0.0);
    end();

    return v;
}

long PreferencesManager::getScaleZeroOffset()
{
    begin(true);
    auto v = preferences.getFloat("zo", 0.0);
    end();

    return v;
}

bool PreferencesManager::isScaleCalibrated()
{
    begin(true);
    bool isCalibrated = preferences.isKey("cf");
    end();
    return isCalibrated;
}