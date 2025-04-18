#include "terminal_api.h"
#include <ArduinoJson.h>

TerminalApi::TerminalApi()
{
    // Default constructor
    wifiManager = nullptr;
}

void TerminalApi::begin(WiFiManager *wifiManager, const char *pat)
{
    // Store WiFiManager and authentication token
    this->wifiManager = wifiManager;
    this->tokenHeader = String("Bearer ");
    this->tokenHeader += pat;
}

std::vector<Product> TerminalApi::getProducts()
{
    std::vector<Product> products;

    wifiManager->reconnect();

    // Prepare request URL
    String url = String(BASE_URL) + "/product";
    String response;

    if (wifiManager->request(url.c_str(), "GET", "", response, tokenHeader.c_str()))
    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);

        if (error)
        {
            Serial.print("JSON parsing failed: ");
            Serial.println(error.c_str());
            return products;
        }

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

// GET /order
std::vector<Order> TerminalApi::getOrders()
{
    std::vector<Order> orders;

    wifiManager->reconnect();

    // Prepare request URL
    String url = String(BASE_URL) + "/order";
    String response;

    if (wifiManager->request(url.c_str(), "GET", "", response, tokenHeader.c_str()))
    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);

        if (error)
        {
            Serial.print("JSON parsing failed: ");
            Serial.println(error.c_str());
            return orders;
        }

        JsonArray data = doc["data"].as<JsonArray>();

        for (JsonVariant orderVariant : data)
        {
            Order order;
            JsonObject orderObj = orderVariant.as<JsonObject>();

            order.id = orderObj["id"].as<String>();
            order.created = orderObj["created"].as<String>();
            order.index = orderObj["index"].as<uint16_t>();

            JsonObject shipping = orderObj["shipping"].as<JsonObject>();
            order.shipping.name = shipping["name"].as<String>();
            order.shipping.street1 = shipping["street1"].as<String>();
            order.shipping.street2 = shipping["street2"].as<String>();
            order.shipping.city = shipping["city"].as<String>();
            order.shipping.province = shipping["province"].as<String>();
            order.shipping.country = shipping["country"].as<String>();
            order.shipping.zip = shipping["zip"].as<String>();
            order.shipping.phone = shipping["phone"].as<String>();

            JsonObject amount = orderObj["amount"].as<JsonObject>();
            order.amount.subtotal = amount["subtotal"].as<uint32_t>();
            order.amount.shipping = amount["shipping"].as<uint32_t>();

            JsonObject tracking = orderObj["tracking"].as<JsonObject>();
            if (tracking["status"].isUnbound() || tracking["number"].isUnbound() || tracking["service"].isUnbound() || tracking["url"].isUnbound())
            {
                // Tracking information is not available
                order.tracking.status = "";
                order.tracking.number = "";
                order.tracking.service = "";
                order.tracking.url = "";
            }
            else
            {
                order.tracking.service = tracking["service"].as<String>();
                order.tracking.number = tracking["number"].as<String>();
                order.tracking.status = tracking["status"].as<String>();
                order.tracking.url = tracking["url"].as<String>();
            }

            JsonArray items = orderObj["items"].as<JsonArray>();
            for (JsonVariant itemVariant : items)
            {
                JsonObject itemObj = itemVariant.as<JsonObject>();
                OrderItem item;

                item.id = itemObj["id"].as<String>();
                item.description = itemObj["description"].as<String>();

                order.items.push_back(item);
            }

            orders.push_back(order);
        }
        return orders;
    }

    Serial.println("Failed to fetch orders");
    return orders;
}

// DELETE /cart
bool TerminalApi::clearCart()
{
    wifiManager->reconnect();

    // Prepare request URL
    String url = String(BASE_URL) + "/cart";
    String response;

    if (wifiManager->request(url.c_str(), "DELETE", "", response, tokenHeader.c_str()))
    {
        Serial.println("Cart cleared successfully");
        return true;
    }

    Serial.println("Failed to clear cart");
    return false;
}

// GET /cart
Cart *TerminalApi::getCart()
{
    wifiManager->reconnect();

    // Prepare request URL
    String url = String(BASE_URL) + "/cart";
    String response;

    if (wifiManager->request(url.c_str(), "GET", "", response, tokenHeader.c_str()))
    {
        Serial.printf("Response: %s\n", response.c_str());
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);

        if (error)
        {
            Serial.print("JSON parsing failed: ");
            Serial.println(error.c_str());
            return nullptr;
        }

        JsonObject data = doc["data"].as<JsonObject>();
        Cart *cart = new Cart();

        cart->subtotal = data["subtotal"].as<uint32_t>();
        cart->addressID = data["addressID"].as<String>();
        cart->cardID = data["cardID"].as<String>();

        JsonArray items = data["items"].as<JsonArray>();
        for (JsonVariant itemVariant : items)
        {
            JsonObject itemObj = itemVariant.as<JsonObject>();
            CartItem item;

            item.id = itemObj["id"].as<String>();
            item.productVariantID = itemObj["productVariantID"].as<String>();
            item.quantity = itemObj["quantity"].as<uint32_t>();
            item.subtotal = itemObj["subtotal"].as<uint32_t>();

            cart->items.push_back(item);
        }

        return cart;
    }

    Serial.println("Failed to fetch cart");
    return nullptr;
}

// PUT /cart/item
Cart *TerminalApi::addItemToCart(const char *productVariantID, uint32_t quantity)
{
    wifiManager->reconnect();

    // Prepare request URL
    String url = String(BASE_URL) + "/cart/item";
    String response;

    // Create JSON payload
    JsonDocument doc;
    doc["productVariantID"] = productVariantID;
    doc["quantity"] = quantity;

    String jsonString;
    serializeJson(doc, jsonString);

    if (wifiManager->request(url.c_str(), "PUT", jsonString.c_str(), response, tokenHeader.c_str()))
    {
        Serial.printf("Response: %s\n", response.c_str());
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);

        if (error)
        {
            Serial.print("JSON parsing failed: ");
            Serial.println(error.c_str());
            return nullptr;
        }

        JsonObject data = doc["data"].as<JsonObject>();
        Cart *cart = new Cart();

        cart->subtotal = data["subtotal"].as<uint32_t>();
        cart->addressID = data["addressID"].as<String>();
        cart->cardID = data["cardID"].as<String>();

        JsonArray items = data["items"].as<JsonArray>();
        for (JsonVariant itemVariant : items)
        {
            JsonObject itemObj = itemVariant.as<JsonObject>();
            CartItem item;

            item.id = itemObj["id"].as<String>();
            item.productVariantID = itemObj["productVariantID"].as<String>();
            item.quantity = itemObj["quantity"].as<uint32_t>();
            item.subtotal = itemObj["subtotal"].as<uint32_t>();

            cart->items.push_back(item);
        }

        return cart;
    }

    Serial.println("Failed to add item to cart");
    return nullptr;
}

// POST /cart
Order *TerminalApi::convertCartToOrder()
{
    wifiManager->reconnect();

    // Prepare request URL
    String url = String(BASE_URL) + "/cart/convert";
    String response;

    if (wifiManager->request(url.c_str(), "POST", "", response, tokenHeader.c_str()))
    {
        Serial.printf("Response: %s\n", response.c_str());
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);

        if (error)
        {
            Serial.print("JSON parsing failed: ");
            Serial.println(error.c_str());
            return nullptr;
        }

        JsonObject data = doc["data"].as<JsonObject>();
        Order *order = new Order();

        order->id = data["id"].as<String>();
        order->index = data["index"].as<uint16_t>();

        JsonObject shipping = data["shipping"].as<JsonObject>();
        order->shipping.name = shipping["name"].as<String>();
        order->shipping.street1 = shipping["street1"].as<String>();
        order->shipping.street2 = shipping["street2"].as<String>();
        order->shipping.city = shipping["city"].as<String>();
        order->shipping.province = shipping["province"].as<String>();
        order->shipping.country = shipping["country"].as<String>();
        order->shipping.zip = shipping["zip"].as<String>();
        order->shipping.phone = shipping["phone"].as<String>();

        JsonObject amount = data["amount"].as<JsonObject>();
        order->amount.subtotal = amount["subtotal"].as<uint32_t>();
        order->amount.shipping = amount["shipping"].as<uint32_t>();

        JsonObject tracking = data["tracking"].as<JsonObject>();
        order->tracking.service = tracking["service"].as<String>();
        order->tracking.number = tracking["number"].as<String>();
        order->tracking.url = tracking["url"].as<String>();

        JsonArray items = data["items"].as<JsonArray>();
        for (JsonVariant itemVariant : items)
        {
            JsonObject itemObj = itemVariant.as<JsonObject>();
            OrderItem item;
            item.id = itemObj["id"].as<String>();
            item.amount = itemObj["amount"].as<uint32_t>();
            item.quantity = itemObj["quantity"].as<uint32_t>();
            item.productVariantID = itemObj["productVariantID"].as<String>();
            order->items.push_back(item);
        }

        return order;
    }

    Serial.println("Failed to convert cart to order");
    return nullptr;
}