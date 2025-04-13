#include "led.h"

LedStrip::LedStrip() : strip(NUM_LEDS, LED_DATA_PIN)
{
}

void LedStrip::begin()
{
    strip.Begin();
    for (int i = 0; i < NUM_LEDS; i++)
    {
        strip.SetPixelColor(i, RgbColor(0, 0, 0));
    }
    strip.Show();
}

// Animation task function
void LedStrip::animationTask(void *parameter)
{
    AnimationData *data = (AnimationData *)parameter;
    LedStrip *strip = (data->instance);
    RgbColor startColor = data->start;
    RgbColor endColor = data->end;
    AnimEaseFunction easeFunction = data->easeFunction;
    uint32_t duration = data->duration;

    NeoPixelAnimator animator(1);
    animator.StartAnimation(0, duration, [=](const AnimationParam &param)
                            {
                                if (param.state == AnimationState_Completed)
                                {
                                    return;
                                }


                                float progress = easeFunction(param.progress);
                                RgbColor color = RgbColor::LinearBlend(startColor, endColor, progress);
                                for (int i = 0; i < NUM_LEDS; i++)
                                {
                                    strip->strip.SetPixelColor(i, color);
                                } });

    while (animator.IsAnimating())
    {
        animator.UpdateAnimations();
        strip->strip.Show();

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    // Clean up
    delete data;
    vTaskDelete(NULL);
}

// Starts the "turn on animation" (fade from black to orange) as a background task
void LedStrip::turnOnAnimation()
{
    AnimationData *data = new AnimationData;
    data->start = RgbColor(0, 0, 0);
    data->end = RgbColor(255, 94, 0); // Orange
    data->easeFunction = NeoEase::CubicOut;
    data->instance = this;
    data->duration = 5000;

    createAnimationTask(data);
}

// Starts the "turn off animation" (fade from orange to black) as a background task
void LedStrip::turnOffAnimation()
{
    AnimationData *data = new AnimationData;
    data->start = RgbColor(255, 94, 0); // Orange
    data->end = RgbColor(0, 0, 0);
    data->easeFunction = NeoEase::CubicInOut;
    data->instance = this;
    data->duration = 2000;

    createAnimationTask(data);
}

void LedStrip::createAnimationTask(AnimationData *data)
{
    if (animationTaskHandle != NULL)
    {
        vTaskDelete(animationTaskHandle);
        animationTaskHandle = NULL;
    }

    BaseType_t result = xTaskCreate(
        animationTask,       
        "AnimationTask",     
        4096,             
        (void *)data,       
        16,                 
        &animationTaskHandle 
    );
    if (result != pdPASS)
    {
        Serial.println("Error: Failed to create animation task");
        delete data;
    }
}

void LedStrip::progress(float percentage) // percentage: 0.0 to 1.0
{
    static const uint8_t maxBrightness = 128;
    Serial.printf("Progress: %.2f\n", percentage);
    percentage = max(0.0f, min(1.0f, percentage));

    int ledCount = (int)(NUM_LEDS * percentage);
    int lastLedBrightness = (int)(maxBrightness * (percentage - (float)ledCount / NUM_LEDS) * NUM_LEDS);

    auto color = RgbColor(255, 94, 0); // Orange

    for (int i = 0; i < NUM_LEDS; i++)
    {
        if (i < ledCount)
        {
            strip.SetPixelColor(i, color);
        }
        else if (i == ledCount)
        {
            if (lastLedBrightness > 0)
            {
                strip.SetPixelColor(i, RgbColor(color.R * lastLedBrightness / maxBrightness,
                                                color.G * lastLedBrightness / maxBrightness,
                                                color.B * lastLedBrightness / maxBrightness));
            }
            else
            {
                strip.SetPixelColor(i, RgbColor(0, 0, 0));
            }
        }
        else
        {
            strip.SetPixelColor(i, RgbColor(0, 0, 0));
        }
    }
    strip.Show();
}