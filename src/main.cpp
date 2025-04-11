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

#define PIN_TERMINAL_BUTTON 17
#define PIN_TOPLEFT 25
#define PIN_TOPMIDDLE 32
#define PIN_TOPRIGHT 33
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
UI ui = UI(tft);
PreferencesManager preferences = PreferencesManager();

#define MAIN_FONT &GeistMono_VariableFont_wght10pt7b

void calibrate();
void listFiles(const char *dirname);

void setup()
{
  preferences.begin();

  // Initialize serial first for debugging
  Serial.begin(115200);
  Serial.println("\n\n=== Scale Application Starting ===");

  // Initialize LittleFS
  if (!LittleFS.begin(false))
  {
    Serial.println("LittleFS mount failed! Formatting...");
    if (!LittleFS.begin(true))
    {
      Serial.println("LittleFS mount failed even after formatting!");
    }
    else
    {
      Serial.println("LittleFS formatted and mounted successfully");
    }
  }
  else
  {
    Serial.println("LittleFS mounted successfully");
  }

  // List files in the root directory to verify
  listFiles("/");

  // Initialize LED strip
  ledStrip.begin();

  // Initialize the display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextWrap(false);

  // Initialize the UI system
  ui.begin();

  // Draw the menu with the PNG images
  ui.drawMenu();

  // Rest of your setup code
  pinMode(PIN_TERMINAL_BUTTON, INPUT_PULLUP);
  pinMode(PIN_TOPLEFT, INPUT_PULLUP);
  pinMode(PIN_TOPMIDDLE, INPUT_PULLUP);
  pinMode(PIN_TOPRIGHT, INPUT_PULLUP);

  scale.begin(PIN_DT, PIN_SCK);

  if (!preferences.isScaleCalibrated())
  {
    Serial.println("Scale not calibrated. Starting calibration mode...");
    calibrate();
  }
  else
  {
    float calibrationFactor = preferences.getScaleCalibrationFactor();
    Serial.printf("Using saved calibration factor: %.2f\n", calibrationFactor);
    scale.set_scale(calibrationFactor);
    scale.tare(); // Reset the scale to 0
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

  // show state of all buttons
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(MAIN_FONT);
  tft.setTextSize(1);

  tft.setCursor(0, 20);
  tft.printf("Left: %s\n", digitalRead(PIN_TOPLEFT) == LOW ? "Pressed" : "Released");
  tft.printf("Middle: %s\n", digitalRead(PIN_TOPMIDDLE) == LOW ? "Pressed" : "Released");
  tft.printf("Right: %s\n", digitalRead(PIN_TOPRIGHT) == LOW ? "Pressed" : "Released");
  tft.printf("Terminal: %s\n", digitalRead(PIN_TERMINAL_BUTTON) == LOW ? "Pressed" : "Released");

  delay(1000);

  return;
  if (digitalRead(PIN_TERMINAL_BUTTON) == LOW)
  {
    if (scale.wait_ready_timeout(200))
    {
      Serial.println("Switch pressed, reading weight...");
      float reading = scale.get_units(20);
      Serial.printf("Weight: %.2f g\n", reading);

      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(2);
      tft.drawCentreString("Weight:", tft.width() / 2, tft.height() / 2 - 20, 2);
      tft.drawCentreString(String(reading, 2) + " g", tft.width() / 2, tft.height() / 2 + 20, 4);
      tft.setTextSize(1);

      delay(1000); // Wait for a second before the next reading
    }
  }
  else
  {
  }
  delay(100); // Small delay to avoid flooding the serial output
}

void calibrate()
{
  scale.set_scale(); // Set the scale to the default calibration factor
  scale.tare();      // Reset the scale to 0

  // Clear the screen and show initial instructions
  tft.fillScreen(TFT_BLACK);

  TextConfig instructionConfig = ui.createTextConfig(MAIN_FONT);
  instructionConfig.y = 40;

  auto bounds = ui.typeText("Calibration Mode", defaultText);
  delay(1000);
  ui.wipeText(bounds);

  ui.typeText("1. Place a known weight", instructionConfig);

  instructionConfig.y = 80;
  auto buttonBounds = ui.typeText("2. Press any button", instructionConfig);

  // Wait for button press
  while (digitalRead(PIN_TOPLEFT) == HIGH &&
         digitalRead(PIN_TOPMIDDLE) == HIGH &&
         digitalRead(PIN_TOPRIGHT) == HIGH &&
         digitalRead(PIN_TERMINAL_BUTTON) == HIGH)
  {
    delay(100);
  }

  ui.wipeText(buttonBounds);
  instructionConfig.y = 80;
  auto measuringBounds = ui.typeText("Measuring...", instructionConfig);

  // Take multiple readings for better accuracy
  long reading = scale.get_units(10);

  ui.wipeText(measuringBounds);

  // Show the reading and prompt for weight input
  char buf[32];
  snprintf(buf, sizeof(buf), "Reading: %ld", reading);
  instructionConfig.y = 80;
  auto readingBounds = ui.typeText(buf, instructionConfig);

  instructionConfig.y = 120;
  ui.typeText("Enter weight in grams", instructionConfig);
  instructionConfig.y = 160;
  ui.typeText("via Serial Monitor", instructionConfig);

  // Wait for serial input
  while (Serial.available() == 0)
  {
    delay(100);
  }

  float knownWeight = Serial.parseFloat();
  float calibrationFactor = reading / knownWeight;

  // Save calibration factor
  preferences.setScaleCalibrationFactor(calibrationFactor);

  // Show completion message
  tft.fillScreen(TFT_BLACK);
  instructionConfig.y = 40;
  bounds = ui.typeText("Calibration Complete!", titleText);

  char factorBuf[32];
  snprintf(factorBuf, sizeof(factorBuf), "Factor: %.2f", calibrationFactor);
  instructionConfig.y = 80;
  ui.typeText(factorBuf, instructionConfig);

  instructionConfig.y = 120;
  auto restartBounds = ui.typeText("Restarting...", instructionConfig);

  // Apply the calibration factor
  scale.set_scale(calibrationFactor);
  scale.tare(); // Reset the scale to 0

  delay(2000); // Show the completion message for 2 seconds
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
  bounds = ui.typeText("Clearing cart...", MAIN_FONT);
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