#include "preferences_manager.h"

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

void PreferencesManager::setScaleCalibrationFactor(float factor)
{
    preferences.putFloat("calibration_factor", factor);
}

void PreferencesManager::setScaleZeroFactor(float factor)
{
    preferences.putFloat("zero_factor", factor);
}

float PreferencesManager::getScaleCalibrationFactor()
{
    return preferences.getFloat("calibration_factor", 0.0);
}

float PreferencesManager::getScaleZeroFactor()
{
    return preferences.getFloat("zero_factor", 0.0);
}

bool PreferencesManager::isScaleCalibrated()
{
    return preferences.isKey("calibration_factor");
}