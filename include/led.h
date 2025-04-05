#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include <NeoPixelBus.h>

#define NUM_LEDS 8
#define LED_DATA_PIN 5

class LedStrip
{
private:
    NeoPixelBus<NeoGrbFeature, NeoWs2812Method> strip;

public:
    LedStrip();
    void begin();

    void progress(float percentage);
};

#endif