#include "terminal_api.h"
#include <ArduinoJson.h>

TerminalApi::TerminalApi()
{
    // Default constructor
    wifiManager = nullptr;
    token = nullptr;
}

void TerminalApi::begin(WiFiManager *wifiManager, const char *pat)
{
    // Store WiFiManager and authentication token
    this->wifiManager = wifiManager;
    this->token = pat;
}

std::vector<Product> TerminalApi::getProducts()
{
    std::vector<Product> products;

    // Make sure WiFi is connected
    if (!wifiManager || !wifiManager->isConnected())
    {
        Serial.println("WiFi not connected or not initialized");
        return products;
    }

    // Prepare request URL
    String url = String(BASE_URL) + "/product";
    String response;

    // Set up headers with authentication token
    String headersStr = "Authorization: Bearer ";
    headersStr += token;

    // Make GET request with headers
    if (wifiManager->request(url.c_str(), "GET", "", response, headersStr.c_str()))
    {
        Serial.printf("Response: %s\n", response.c_str());
        Serial.printf("Response length: %d\n", response.length());

                // Parse JSON response
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);

        if (error)
        {
            Serial.print("JSON parsing failed: ");
            Serial.println(error.c_str());
            return products;
        }

        // Extract products from JSON
        JsonArray data = doc["data"].as<JsonArray>();

        for (JsonVariant productVariant : data)
        {
            Product product;
            JsonObject productObj = productVariant.as<JsonObject>();

            product.id = productObj["id"].as<String>();
            product.name = productObj["name"].as<String>();
            product.description = productObj["description"].as<String>();
            product.order = productObj["order"].as<uint16_t>();
            product.subscription = productObj["subscription"].as<String>();

            // Extract variants
            JsonArray variants = productObj["variants"].as<JsonArray>();
            for (JsonVariant variantVariant : variants)
            {
                JsonObject variantObj = variantVariant.as<JsonObject>();
                Variant variant;

                variant.id = variantObj["id"].as<String>();
                variant.name = variantObj["name"].as<String>();
                variant.price = variantObj["price"].as<uint32_t>();

                product.variants.push_back(variant);
            }

            products.push_back(product);
        }
    }
    else
    {
        Serial.println("Failed to fetch products");
    }

    return products;
}