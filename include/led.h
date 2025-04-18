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

    void turnOff();

    void progress(float percentage, RgbColor color = RgbColor(255, 94, 0));
    void scrollIndicator(uint index, uint size, RgbColor color = RgbColor(255, 94, 0));

    void turnOnAnimation();
    void turnOffAnimation();

    void purchaseAnimation();

    void reorderAnimation();

    static void animationTask(void *parameter);
};

struct AnimationData
{
    LedStrip *instance;
    RgbColor start;
    RgbColor end;
    AnimEaseFunction easeFunction;
    uint32_t duration;
    bool reverse = false;
    bool loop = false;
};

#endif