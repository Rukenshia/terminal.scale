#include "wifi_manager.h"
#include <HTTPClient.h>

const char *amazon_root_ca1 =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
    "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
    "-----END CERTIFICATE-----\n";

WiFiManager::WiFiManager()
{
}

void WiFiManager::begin(const char *ssid, const char *password)
{
    // Store credentials and initialize WiFi
    WiFi.begin(ssid, password);

    // Set up secure client with root certificate
    client.setCACert(amazon_root_ca1);
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

bool WiFiManager::request(const char *url, const char *method, const char *body, String &response, const char *headers)
{
    // Parse URL to extract host and path
    String hostName = "";
    String path = "/";

    String urlStr = String(url);
    int protocolEnd = urlStr.indexOf("://");
    if (protocolEnd > 0)
    {
        urlStr = urlStr.substring(protocolEnd + 3);
    }

    int pathStart = urlStr.indexOf('/');
    if (pathStart > 0)
    {
        hostName = urlStr.substring(0, pathStart);
        path = urlStr.substring(pathStart);
    }
    else
    {
        hostName = urlStr;
    }

    client.setCACert(amazon_root_ca1);

    Serial.printf("Connecting to %s\n", hostName.c_str());
    Serial.printf("Path: %s\n", path.c_str());

    if (!client.connect(hostName.c_str(), 443))
    {
        Serial.println("Connection failed");
        return false;
    }

    // Build the HTTP request
    String httpRequest = String(method) + " " + path + " HTTP/1.1\r\n" +
                         "Host: " + hostName + "\r\n" +
                         "Content-Type: application/json\r\n";

    // Add custom headers if provided
    if (headers != nullptr && strlen(headers) > 0)
    {
        httpRequest += String(headers) + "\r\n";
    }

    httpRequest += "Content-Length: " + String(strlen(body)) + "\r\n" +
                   "Connection: close\r\n\r\n" +
                   body;

    // Send the request
    client.print(httpRequest);

    // Wait for response
    unsigned long timeout = millis();
    while (client.connected() && !client.available())
    {
        if (millis() - timeout > 5000)
        {
            Serial.println("Client Timeout");
            client.stop();
            return false;
        }
        delay(10);
    }

    // Read the response
    response = "";
    while (client.available())
    {
        response += client.readString();
    }

    // Parse the response to extract the body
    bool successStatus = false;
    int statusCodeStart = response.indexOf("HTTP/1.1 ");
    if (statusCodeStart != -1)
    {
        statusCodeStart += 9; // Move past "HTTP/1.1 "
        int statusCodeEnd = response.indexOf(' ', statusCodeStart);
        if (statusCodeEnd != -1)
        {
            String statusCodeStr = response.substring(statusCodeStart, statusCodeEnd);
            int statusCode = statusCodeStr.toInt();
            if (statusCode >= 200 && statusCode < 300)
            {
                successStatus = true;
            }
        }
    }
    int bodyStart = response.indexOf("\r\n\r\n");
    if (bodyStart != -1)
    {
        response = response.substring(bodyStart + 4);
    }

    client.stop();

    return true;
}