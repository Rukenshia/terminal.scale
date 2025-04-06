#include <Arduino.h>
#include <HX711.h>

#include <SPI.h>
#include <TFT_eSPI.h>

#include "ui.h"
#include "led.h"
#include "wifi.secret.h"
#include "wifi_manager.h"
#include "terminal_api.h"

#define PIN_SWITCH 17
#define PIN_DT 27
#define PIN_SCK 26

#define PIN_DIN 19
#define PIN_CLK 18
#define PIN_CS 16
#define PIN_DC 4
#define PIN_RST 0
#define PIN_BL 2

#define CALIBRATION_MODE 0

static const float CALIBRATION_FACTOR = 1103.88; // This value is obtained from the calibration process
HX711 scale;

TFT_eSPI tft = TFT_eSPI();
LedStrip ledStrip = LedStrip();
WiFiManager wifi = WiFiManager();
TerminalApi terminalApi = TerminalApi();

void calibrate();

void setup()
{

  Serial.begin(115200);

  ledStrip.begin();

  for (float progress = 0.0f; progress <= 1.0f; progress += 0.01f)
  {
    ledStrip.progress(progress);
    delay(10);
  }
  delay(5000);
  ledStrip.progress(0.0f);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  auto bounds = typeText(tft, "Initializing", &GeistMono_VariableFont_wght18pt7b, 150);
  startBlinking(tft, bounds, 1000);

  wifi.begin(WIFI_SSID, WIFI_PASSWORD);
  wifi.connect();
  wifi.syncTime();
  Serial.println("WiFi connected");

  stopBlinking();
  wipeText(tft, bounds);

  terminalApi.begin(&wifi, "trm_test_5a684b12979177c46aac");

  Serial.println("Fetching products...");
  std::vector<Product> products = terminalApi.getProducts();
  Serial.printf("Found %d products\n", products.size());

  // show first product
  bounds = typeText(tft, "Product: ", &GeistMono_VariableFont_wght18pt7b, 150);
  delay(1000);
  wipeText(tft, bounds);
  tft.setTextColor(ACCENT_COLOR);
  typeText(tft, products[0].name.c_str(), &GeistMono_VariableFont_wght18pt7b, 150);
  delay(5000);
  wipeText(tft, bounds);

  // clear cart
  typeText(tft, "Clearing cart...", &GeistMono_VariableFont_wght18pt7b, 150);
  if (terminalApi.clearCart())
  {
    Serial.println("Cart cleared");
  }
  else
  {
    Serial.println("Failed to clear cart");
  }
  wipeText(tft, bounds);

  // add first product to cart
  typeText(tft, "Adding to cart...", &GeistMono_VariableFont_wght18pt7b, 150);
  Cart *cart = terminalApi.addItemToCart(products[0].variants[0].id.c_str(), 1);
  if (cart)
  {
    Serial.println("Item added to cart");
    Serial.printf("Subtotal: %d\n", cart->subtotal);
    Serial.printf("Address ID: %s\n", cart->addressID.c_str());
    Serial.printf("Card ID: %s\n", cart->cardID.c_str());

    wipeText(tft, bounds);
    tft.setTextColor(ACCENT_COLOR);
    typeText(tft, "Cart subtotal: ", &GeistMono_VariableFont_wght18pt7b, 150);
    tft.setTextColor(TFT_WHITE);
    typeText(tft, String(cart->subtotal).c_str(), &GeistMono_VariableFont_wght18pt7b, 150, 0, bounds.y + bounds.height);
    delay(5000);
  }
  else
  {
    Serial.println("Failed to add item to cart");
  }
  wipeText(tft, bounds);

  // place order
  typeText(tft, "Placing order...", &GeistMono_VariableFont_wght18pt7b, 150);
  Order *order = terminalApi.convertCartToOrder();
  if (order)
  {
    Serial.println("Order placed");
    Serial.printf("Order ID: %s\n", order->id.c_str());
    Serial.printf("Shipping address ID: %s\n", order->shipping.id.c_str());
    Serial.printf("Tracking number: %s\n", order->tracking.number.c_str());
    Serial.printf("Tracking URL: %s\n", order->tracking.url.c_str());

    wipeText(tft, bounds);
    tft.setTextColor(ACCENT_COLOR);
    typeText(tft, "Order ID: ", &GeistMono_VariableFont_wght18pt7b, 150);
    tft.setTextColor(TFT_WHITE);
    typeText(tft, order->id.c_str(), &GeistMono_VariableFont_wght18pt7b, 150, 0, bounds.y + bounds.height);
    delay(5000);
  }
  else
  {
    Serial.println("Failed to place order");
  }
  wipeText(tft, bounds);

  // terminalAnimation(tft);

  pinMode(PIN_SWITCH, INPUT);

  scale.begin(PIN_DT, PIN_SCK);

#if CALIBRATION_MODE
  calibrate();
#else
  Serial.printf("Calibration factor: %.2f\n", CALIBRATION_FACTOR);
  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare(); // Reset the scale to 0
#endif
}

void loop()
{

  if (digitalRead(PIN_SWITCH) == HIGH)
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

  // Debug code to get scale calibration factor
  Serial.println("Place a known weight on the scale.");
  Serial.println("Press the switch to start calibration.");

  while (digitalRead(PIN_SWITCH) == LOW)
  {
    // Wait for switch to be pressed
  }
  Serial.println("Switch pressed, starting calibration...");
  long reading = scale.get_units(10);

  Serial.print("Reading: ");
  Serial.println(reading);

  Serial.println("Enter the known weight in grams:");
  while (Serial.available() == 0)
  {
    // Wait for user input
  }
  float knownWeight = Serial.parseFloat();
  Serial.print("Known weight: ");
  Serial.println(knownWeight);
  float calibrationFactor = reading / knownWeight;

  Serial.print("Calibration factor: ");
  Serial.println(calibrationFactor);
}
