#include "wifi_manager.h"

WiFiManager::WiFiManager()
{
}

void WiFiManager::begin(const char *ssid, const char *password)
{
    this->ssid = ssid;
    this->password = password;

    // Store credentials and initialize WiFi
    WiFi.begin(ssid, password);
}

void WiFiManager::connect()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Already connected to WiFi");
        return;
    }

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        if (attempts > 20)
        {
            Serial.println("resetting connection");
            WiFi.disconnect();
            delay(5000);
            Serial.println("reconnecting");
            WiFi.begin();

            attempts = 0;
        }
        Serial.print(".");
        delay(1000);
        attempts++;
    }

    Serial.println("Connected to WiFi");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
}

void WiFiManager::disconnect()
{
    // Disconnect from WiFi
    WiFi.disconnect();
}

bool WiFiManager::isConnected()
{
    // Check if connected to WiFi
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::syncTime()
{
    // Synchronize time with NTP server
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 1000))
    {
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
}

bool WiFiManager::request(const char *url, const char *method, const char *body, String &response, const char *token)
{
    // Make HTTP request
    HTTPClient client;

    client.begin(url);
    client.addHeader("Content-Type", "application/json");
    if (token)
    {
        client.addHeader("Authorization", token);
    }

    int httpCode = client.sendRequest(method, body);
    if (httpCode > 0)
    {
        response = client.getString();
        Serial.printf("HTTP response code: %d\n", httpCode);
        Serial.printf("Response: %s\n", response.c_str());
    }
    else
    {
        Serial.printf("HTTP request failed, error: %s\n", client.errorToString(httpCode).c_str());
        return false;
    }
    if (httpCode == HTTP_CODE_OK)
    {
        Serial.println("Request succeeded");
    }
    else
    {
        Serial.printf("Request failed with code: %d\n", httpCode);
        return false;
    }

    // Check if the response is valid
    if (response.length() == 0)
    {
        Serial.println("Empty response");
        return false;
    }

    return true;
}