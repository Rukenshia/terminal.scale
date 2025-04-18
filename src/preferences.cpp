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

void PreferencesManager::deleteCalibrationData()
{
    begin();
    preferences.remove("cf");
    preferences.remove("zo");
    end();
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

bool PreferencesManager::isConfigured()
{
    begin(true);
    bool configured = preferences.isKey("auto");
    Serial.printf("Configured: %d\n", configured);
    end();
    return configured;
}

bool PreferencesManager::shouldReorderAutomatically()
{
    begin();
    bool value = preferences.getBool("auto", false);
    end();
    return value;
}

void PreferencesManager::setShouldReorderAutomatically(bool value)
{
    begin();
    preferences.putBool("auto", value);
    end();
}

bool PreferencesManager::doNotReorder()
{
    begin(true);
    bool value = preferences.getBool("block", false);
    end();
    return value;
}

void PreferencesManager::setDoNotReorder(bool value)
{
    begin();
    preferences.putBool("block", value);
    end();
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

String PreferencesManager::getCoffeeBagName()
{
    begin(true);
    String bagName = preferences.getString("bag_name", "");
    end();
    return bagName;
}

void PreferencesManager::setCoffeeBagName(const String &name)
{
    begin();
    preferences.putString("bag_name", name);
    end();
}
