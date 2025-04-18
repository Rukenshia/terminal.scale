#include <Arduino.h>
#include <HX711.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <LittleFS.h>
#include "ui.h"
#include "led.h"
#include "wifi.secret.h"
#include "wifi_manager.h"
#include "terminal_api.h"
#include "preferences_manager.h"
#include "buttons.h"
#include "scale.h"
#include "store.h"
#include "debug.h"

#define PIN_DT 27
#define PIN_SCK 26

#define PIN_DIN 19
#define PIN_CLK 18
#define PIN_CS 16
#define PIN_DC 4
#define PIN_RST 0
#define PIN_BL 2

HX711 scale;

PreferencesManager preferences = PreferencesManager();
TFT_eSPI tft = TFT_eSPI();
LedStrip ledStrip = LedStrip();
WiFiManager wifi = WiFiManager();
TerminalApi terminalApi = TerminalApi();
UI ui = UI(tft, ledStrip, terminalApi, preferences);
Scale scaleManager(scale, tft, ui, preferences, terminalApi, PIN_DT, PIN_SCK);

void listFiles(const char *dirname);

void setup()
{
  Serial.begin(115200);

  // Initialize the display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextWrap(false);

  // Initialize LittleFS
  if (!LittleFS.begin(false))
  {
    Serial.println("LittleFS mount failed!");
    return;
  }

  // Initialize LED strip
  ledStrip.begin();

#ifdef LED_SCROLL_INDICATOR_DEBUG
  for (int i = 0; i < 10; i++)
  {
    ledStrip.scrollIndicator(i, 10);
    delay(1000);
  }
  ledStrip.turnOff();
#endif

  // Initialize the UI system
  ui.begin(&scaleManager);

  // Initialize the scale manager
  scaleManager.begin();

  auto bounds = ui.typeText("Connecting...", titleText);
  ui.startBlinking();
#ifndef NO_WIFI
  wifi.begin(WIFI_SSID, WIFI_PASSWORD);
  wifi.connect();
  terminalApi.begin(&wifi, TERMINAL_PAT);
#endif
  ui.wipeText(bounds);
  ui.stopBlinking();
  if (wifi.isConnected())
  {
    Serial.println("WiFi connected");
  }
  else
  {
    Serial.println("Failed to connect to WiFi");
  }

  if (!scaleManager.isCalibrated())
  {
    Serial.println("Scale not calibrated. Starting calibration mode...");
    scaleManager.calibrate();
  }

#ifndef FAST_STARTUP
  ui.terminalAnimation();
#endif

  if (!preferences.isConfigured())
  {
    ui.beginConfiguration();
  }

  Serial.println("Startup complete");
  ui.menu->selectMenu(MAIN_MENU);
  ui.menu->taint();
  ui.store->taint();
  Serial.println("Startup complete - menu selected");
}

void loop()
{
#ifdef WEIGHING_UI_DEBUG
  for (float f = 0; f < TERMINAL_COFFEE_WEIGHT; f += 1.0f)
  {
    ui.drawWeight(f);
    delay(100);
  }
#endif

#ifdef SERIAL_LISTEN
  if (Serial.available())
  {
    String input = Serial.readStringUntil('\n');
    if (input.startsWith("bag_name="))
    {
      String bagName = input.substring(9);
      preferences.setCoffeeBagName(bagName);
      scaleManager.bagName = bagName;

      Serial.printf("Updated bag name to: %s\n", bagName.c_str());
    }

    if (input.startsWith("calibrate"))
    {
      Serial.println("Resetting calibration");
      preferences.deleteCalibrationData();
      esp_restart();
    }
  }
#endif

  if (scaleManager.checkCalibrationRequest())
  {
    return;
  }

  ui.menu->checkButtonEvents();

  ui.loop();
}
