#include "wifi_manager.h"

WiFiManager::WiFiManager()
{
    // Initialize pointers to prevent null pointer issues
    ssid = NULL;
    password = NULL;
}

void WiFiManager::begin(const char *ssid, const char *password)
{
    this->ssid = ssid;
    this->password = password;

    // Don't auto-connect yet, we'll handle the connection in connect()
    WiFi.persistent(true);
}

void WiFiManager::connect()
{
    // Safety check for null credentials
    if (!ssid || !password)
    {
        Serial.println("Error: WiFi credentials not set. Call begin() first.");
        return;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("Already connected to WiFi");
        return;
    }

    // Complete reset of WiFi
    WiFi.disconnect(true, true); // Disconnect and clear configs
    WiFi.mode(WIFI_OFF);         // Turn WiFi off
    delay(500);                  // Give it time to reset

    // Now initialize WiFi properly
    WiFi.mode(WIFI_STA); // Set to station mode

    // Turn off power-saving mode for faster connections
    // esp_wifi_set_ps(WIFI_PS_NONE);

    Serial.print("Connecting to WiFi");

    // Begin connection with fresh start
    WiFi.begin(ssid, password);

    int attempts = 0;
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime < 20000))
    {
        Serial.print(".");
        delay(500);
        attempts++;

        // If we've tried for a few seconds and still not connected, try a full reconnect
        if (attempts == 10)
        {
            Serial.println("\nRetrying connection with full reset...");
            WiFi.disconnect(true, true);
            delay(1000);
            WiFi.mode(WIFI_STA);
            delay(500);
            WiFi.begin(ssid, password);
        }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nConnected to WiFi");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        WiFi.setAutoReconnect(true); // Enable auto reconnect now that we're connected
    }
    else
    {
        Serial.println("\nFailed to connect to WiFi");
    }
}

void WiFiManager::disconnect()
{
    WiFi.disconnect();
}

bool WiFiManager::isConnected()
{
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