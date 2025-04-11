#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include <NeoPixelBusLg.h>
#include <NeoPixelAnimator.h>

#define NUM_LEDS 8
#define LED_DATA_PIN 5

struct AnimationData;

class LedStrip
{
private:
    NeoPixelBusLg<NeoGrbFeature, NeoWs2812Method> strip;

    TaskHandle_t animationTaskHandle = NULL;
    NeoPixelAnimator animator = NeoPixelAnimator(1);

    void createAnimationTask(AnimationData *data);

public:
    LedStrip();
    void begin();

    void progress(float percentage);
    void turnOnAnimation();
    void turnOffAnimation();

    static void animationTask(void *parameter);
};

struct AnimationData
{
    LedStrip *instance;
    RgbColor start;
    RgbColor end;
    AnimEaseFunction easeFunction;
    uint32_t duration;
};

#endif