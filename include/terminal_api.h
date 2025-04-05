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

class TerminalApi
{
private:
    WiFiManager *wifiManager;

    const char *token;

public:
    TerminalApi();
    void begin(WiFiManager *wifiManager, const char *pat);

    std::vector<Product> getProducts();
};

#endif