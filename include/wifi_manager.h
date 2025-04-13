#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

class WiFiManager
{
private:
    const char *ssid;
    const char *password;

public:
    WiFiManager();
    void begin(const char *ssid, const char *password);
    void connect();
    void reconnect();
    void disconnect();
    bool isConnected();

    void syncTime();

    bool request(const char *url, const char *method, const char *body, String &response, const char *headers = nullptr);
};

#endif