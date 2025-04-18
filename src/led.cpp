#include "led.h"

LedStrip::LedStrip() : strip(NUM_LEDS, LED_DATA_PIN)
{
}

void LedStrip::begin()
{
    strip.Begin();
    turnOff();
}

void LedStrip::turnOff()
{
    if (animationTaskHandle != NULL)
    {
        vTaskDelete(animationTaskHandle);
        animationTaskHandle = NULL;
    }

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
    bool reverse = data->reverse;

    auto anim = [=]()
    {
        // FIXME: the animator should really be a member of the class and act outside of the task
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

        if (reverse)
        {
            // Reverse the animation
            animator.StartAnimation(0, duration, [=](const AnimationParam &param)
                                    {
                                    if (param.state == AnimationState_Completed)
                                    {
                                        return;
                                    }

                                    float progress = easeFunction(param.progress);
                                    RgbColor color = RgbColor::LinearBlend(endColor, startColor, progress);
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
        }
    };

    if (data->loop)
    {
        while (true)
        {
            anim();
        }
    }
    else
    {
        anim();
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

void LedStrip::purchaseAnimation()
{
    AnimationData *data = new AnimationData;
    data->start = RgbColor(0, 0, 0);
    data->end = RgbColor(0, 255, 0);
    data->easeFunction = NeoEase::CubicInOut;
    data->instance = this;
    data->duration = 2000;
    data->reverse = true;

    createAnimationTask(data);
}

void LedStrip::reorderAnimation()
{
    AnimationData *data = new AnimationData;
    data->start = RgbColor(0, 0, 0);
    data->end = RgbColor(255 / 5, 0, 0);
    data->easeFunction = NeoEase::CubicInOut;
    data->instance = this;
    data->duration = 1000;
    data->reverse = true;
    data->loop = true;

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
        &animationTaskHandle);
    if (result != pdPASS)
    {
        Serial.println("Error: Failed to create animation task");
        delete data;
    }
}

void LedStrip::progress(float percentage, RgbColor color) // percentage: 0.0 to 1.0
{
    static const uint8_t maxBrightness = 128;
    Serial.printf("Progress: %.2f\n", percentage);
    percentage = max(0.0f, min(1.0f, percentage));

    int ledCount = (int)(NUM_LEDS * percentage);
    int lastLedBrightness = (int)(maxBrightness * (percentage - (float)ledCount / NUM_LEDS) * NUM_LEDS);

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

void LedStrip::reverseProgress(float percentage, RgbColor color) // percentage: 0.0 to 1.0
{
    // Light LEDs up from right to left
    static const uint8_t maxBrightness = 128;
    Serial.printf("Reverse progress: %.2f\n", percentage);
    percentage = max(0.0f, min(1.0f, percentage));
    int ledCount = (int)(NUM_LEDS * percentage);
    int lastLedBrightness = (int)(maxBrightness * (percentage - (float)ledCount / NUM_LEDS) * NUM_LEDS);

    for (int i = NUM_LEDS - 1; i >= 0; i--)
    {
        if (i > NUM_LEDS - ledCount - 1)
        {
            strip.SetPixelColor(i, color);
        }
        else if (i == NUM_LEDS - ledCount - 1)
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

void LedStrip::scrollIndicator(uint index, uint size, RgbColor color)
{
    // get the LED relative to the start of the strip
    float percent = (float)index / (size - 1);
    auto targetLed = (int)((NUM_LEDS - 1) * percent);

    Serial.printf("Scroll indicator: %d/%d (%f) -> %d\n", index, size, percent, targetLed);

    for (int i = 0; i < NUM_LEDS; i++)
    {
        if (i == targetLed)
        {
            strip.SetPixelColor(i, color);
        }
        else if (i < targetLed)
        {
            auto newColor = RgbColor::LinearBlend(
                color,
                RgbColor(0, 0, 0),
                0.9f);

            strip.SetPixelColor(i, newColor);
        }
        else
        {
            strip.SetPixelColor(i, RgbColor(0, 0, 0));
        }
    }
    strip.Show();
}

void LedStrip::reorderIndication()
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        strip.SetPixelColor(i, RgbColor(16, 2, 0));
    }
    strip.Show();
    showingReorderIndicator = true;
}