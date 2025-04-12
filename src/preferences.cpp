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
    begin();
    preferences.putLong("zo", zeroOffset);
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
    auto v = preferences.getLong("zo", 0.0);
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

bool PreferencesManager::hasCoffeeBag()
{
    begin(true);
    bool hasCoffeeBag = preferences.getBool("bag", false);
    end();
    return hasCoffeeBag;
}

void PreferencesManager::setHasCoffeeBag(bool hasCoffeeBag)
{
    begin();
    preferences.putBool("bag", hasCoffeeBag);
    end();
}