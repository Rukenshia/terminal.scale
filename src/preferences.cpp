#include "preferences.h"

PreferencesManager::PreferencesManager()
{
}

void PreferencesManager::begin()
{
    preferences.begin("scale", false);
}

void PreferencesManager::end()
{
    preferences.end();
}

bool PreferencesManager::isScaleCalibrated()
{
    return preferences.getBool("calibrated", false);
}

void PreferencesManager::setScaleCalibrated(bool calibrated)
{
    preferences.putBool("calibrated", calibrated);
}
