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

TFT_eSPI tft = TFT_eSPI();
LedStrip ledStrip = LedStrip();
WiFiManager wifi = WiFiManager();
TerminalApi terminalApi = TerminalApi();
UI ui = UI(tft, &ledStrip);
PreferencesManager preferences = PreferencesManager();
Scale scaleManager(scale, tft, ui, preferences, PIN_DT, PIN_SCK);

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

  // Initialize the UI system
  ui.begin(&scaleManager);

  // Initialize the scale manager
  scaleManager.begin();

  if (!scaleManager.isCalibrated())
  {
    Serial.println("Scale not calibrated. Starting calibration mode...");
    scaleManager.calibrate();
  }
  else
  {
#ifndef FAST_STARTUP
    ui.terminalAnimation();
#endif

    ui.menu->selectMenu(MAIN_MENU);
    ui.menu->draw();
  }
}

// Helper function to list files recursively
void listFiles(const char *dirname)
{
  Serial.printf("Listing directory: %s\n", dirname);

  fs::File root = LittleFS.open(dirname);
  if (!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  fs::File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      // Recursive call to list subdirectory
      listFiles(file.name());
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void loop()
{
  // Check for calibration requests first
  if (scaleManager.checkCalibrationRequest())
  {
    // If calibration was performed, we don't need to do anything else in this loop iteration
    return;
  }

  // Process any pending button events - this is safer than handling in interrupts
  ui.menu->checkButtonEvents();

  ui.loop();
}

void order()
{
  Serial.println("Fetching products...");
  std::vector<Product> products = terminalApi.getProducts();
  Serial.printf("Found %d products\n", products.size());

  // Simple example with just text and position
  auto bounds = ui.typeText("Product: ");
  delay(1000);
  ui.wipeText(bounds);

  ui.typeText(products[0].name.c_str(), accentText);
  delay(5000);
  ui.wipeText(bounds);

  // clear cart - simplified call
  bounds = ui.typeText("Clearing cart...", titleText);
  if (terminalApi.clearCart())
  {
    Serial.println("Cart cleared");
  }
  else
  {
    Serial.println("Failed to clear cart");
  }
  ui.wipeText(bounds);

  bounds = ui.typeText("Adding to cart...");
  Cart *cart = terminalApi.addItemToCart(products[0].variants[0].id.c_str(), 1);
  if (cart)
  {
    Serial.println("Item added to cart");
    Serial.printf("Subtotal: %d\n", cart->subtotal);
    Serial.printf("Address ID: %s\n", cart->addressID.c_str());
    Serial.printf("Card ID: %s\n", cart->cardID.c_str());

    ui.wipeText(bounds);

    bounds = ui.typeText("Cart subtotal: ");

    TextConfig subtotalConfig = ui.createTextConfig(MAIN_FONT);
    subtotalConfig.y = bounds.y + bounds.height + 8;
    subtotalConfig.enableCursor = false;
    auto bounds2 = ui.typeText(String(cart->subtotal).c_str(), subtotalConfig);
    ui.wipeText(bounds2);
    ui.wipeText(bounds);

    delay(5000);
  }
  else
  {
    Serial.println("Failed to add item to cart");
  }
  ui.wipeText(bounds);

  // place order
  bounds = ui.typeText("Placing order...");
  Order *order = terminalApi.convertCartToOrder();
  if (order)
  {
    Serial.println("Order placed");
    Serial.printf("Order ID: %s\n", order->id.c_str());
    Serial.printf("Shipping address ID: %s\n", order->shipping.id.c_str());
    Serial.printf("Tracking number: %s\n", order->tracking.number.c_str());
    Serial.printf("Tracking URL: %s\n", order->tracking.url.c_str());

    ui.wipeText(bounds);

    ui.typeText("Order placed", accentText);

    delay(5000);
  }
  else
  {
    Serial.println("Failed to place order");
  }
  ui.wipeText(bounds);
}