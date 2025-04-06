#ifndef TERMINAL_API_H
#define TERMINAL_API_H

#include <Arduino.h>
#include <vector>
#include "wifi_manager.h"

#define BASE_URL "https://api.dev.terminal.shop"

template <typename T>
struct ApiResponse
{
    T data;
};

struct Variant
{
    String id;
    String name;
    uint32_t price;
};

struct Product
{
    String id;
    String name;
    String description;
    std::vector<Variant> variants;
    uint16_t order;
    String subscription;
};

struct ShippingAddress
{
    String id;
    String name;
    String street1;
    String street2;
    String city;
    String province;
    String zip;
    String country;
    String phone;
};

struct CartItem
{
    String id;
    String productVariantID;
    uint32_t quantity;
    uint32_t subtotal;
};

struct CartAmount
{
    uint32_t subtotal;
    uint32_t shipping;
    uint32_t total;
};

struct CartShipping
{
    String service;
    String timeframe;
};

struct Cart
{
    std::vector<CartItem> items;
    uint32_t subtotal;
    String addressID;
    String cardID;
    CartAmount amount;
    CartShipping shipping;
};

struct OrderTracking
{
    String service;
    String number;
    String url;
};

struct OrderAmount
{
    uint32_t subtotal;
    uint32_t shipping;
};

struct OrderItem
{
    String id;
    String description;
    uint32_t amount;
    uint32_t quantity;
    String productVariantID;
};

struct Order
{
    String id;
    uint32_t index;
    ShippingAddress shipping;
    OrderAmount amount;
    OrderTracking tracking;
    std::vector<OrderItem> items;
};

class TerminalApi
{
private:
    WiFiManager *wifiManager;

    String tokenHeader;

public:
    TerminalApi();
    void begin(WiFiManager *wifiManager, const char *pat);

    std::vector<Product> getProducts();
    std::vector<ShippingAddress> getShippingAddresses();

    Cart *getCart();
    Cart *createCart();
    Cart *addItemToCart(const char *productVariantID, uint32_t quantity);
    bool clearCart();
    Order *convertCartToOrder();
};

#endif