#include "led.h"

LedStrip::LedStrip() : strip(NUM_LEDS, LED_DATA_PIN)
{
}
void LedStrip::begin()
{
    strip.Begin();
    for (int i = 0; i < NUM_LEDS; i++)
    {
        strip.SetPixelColor(i, RgbColor(0, 0, 0)); // Off
    }
    strip.Show();
}

void LedStrip::progress(float percentage) // percentage: 0.0 to 1.0
{
    static const uint8_t maxBrightness = 128;
    Serial.printf("Progress: %.2f\n", percentage);
    // Ensure percentage is within bounds
    percentage = max(0.0f, min(1.0f, percentage));

    int ledCount = (int)(NUM_LEDS * percentage);
    // calculate last LED brightness (if percentage is between two leds)
    int lastLedBrightness = (int)(maxBrightness * (percentage - (float)ledCount / NUM_LEDS) * NUM_LEDS);

    Serial.printf("LED Count: %d, Last LED Brightness: %d\n", ledCount, lastLedBrightness);

    for (int i = 0; i < NUM_LEDS; i++)
    {
        if (i < ledCount)
        {
            strip.SetPixelColor(i, RgbColor(0, maxBrightness, 0)); // Green
        }
        else if (i == ledCount)
        {
            // Set the last LED to a gradient color
            if (lastLedBrightness > 0)
            {
                strip.SetPixelColor(i, RgbColor(0, lastLedBrightness, 0)); // Gradient green
            }
            else
            {
                strip.SetPixelColor(i, RgbColor(0, 0, 0)); // Off
            }
        }
        else
        {
            strip.SetPixelColor(i, RgbColor(0, 0, 0)); // Off
        }
    }
    strip.Show();
}