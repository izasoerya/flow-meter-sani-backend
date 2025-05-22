#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class WiFiService
{
private:
    const char *ssid = "Subhanallah";
    const char *password = "muhammadnabiyullah";
    const char *firebaseProjectId = "flow-meter-sani";
    const char *collection = "flowmeter";
    const char *documentPath = "0000-0001";
    const char *deviceName = "Flowmeter-0";

public:
    WiFiService() {};
    ~WiFiService() {};

    void connect()
    {
        Serial.print("Connecting to WiFi: ");
        Serial.println(ssid);
        WiFi.begin(ssid, password);

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(1000);
            Serial.print(".");
        }
        Serial.println("\nConnected to WiFi!");
    }

    void reconnect()
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.print("Reconnecting to WiFi: ");
            Serial.println(ssid);
            WiFi.disconnect();
            WiFi.begin(ssid, password);

            while (WiFi.status() != WL_CONNECTED)
            {
                delay(1000);
                Serial.print(".");
            }
            Serial.println("\nReconnected to WiFi!");
        }
    }

    int getDocument(HTTPClient &http, WiFiClientSecure &wClient)
    {
        char url[256];
        snprintf(url, sizeof(url),
                 "https://firestore.googleapis.com/v1/projects/%s/databases/(default)/documents/%s/%s/logs",
                 firebaseProjectId, collection, documentPath);

        http.begin(wClient, url);

        int httpResponseCode = http.GET();
        int maxLogId = -1;

        if (httpResponseCode == 200)
        {
            String response = http.getString();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, response);

            if (!error && doc.containsKey("documents"))
            {
                JsonArray documents = doc["documents"].as<JsonArray>();

                for (JsonObject entry : documents)
                {
                    if (entry.containsKey("fields") && entry["fields"].containsKey("logId"))
                    {
                        int currentLogId = entry["fields"]["logId"]["integerValue"].as<String>().toInt();
                        if (currentLogId > maxLogId)
                        {
                            maxLogId = currentLogId;
                        }
                    }
                }
            }
            else
            {
                Serial.println("Failed to parse or missing 'documents' field.");
                return 0;
            }
        }
        else
        {
            Serial.printf("HTTP Error Code: %d\n", httpResponseCode);
            Serial.println(http.getString());
        }

        http.end();
        return maxLogId;
    }

    int createDocument(HTTPClient &http, WiFiClientSecure &wClient, JsonDocument &jsonDoc)
    {
        char url[256];
        snprintf(url, sizeof(url),
                 "https://firestore.googleapis.com/v1/projects/%s/databases/(default)/documents/%s/%s/logs/",
                 firebaseProjectId, collection, documentPath);
        http.begin(wClient, "https://firestore.googleapis.com/v1/projects/flow-meter-sani/databases/(default)/documents/flowmeter/0000-0001/logs/");
        int httpResponseCode = http.POST(jsonDoc.as<String>());
        return httpResponseCode;
    }

    byte updateDocument(HTTPClient &http, WiFiClientSecure &wClient, JsonDocument &jsonDoc)
    {
        char url[256];
        snprintf(url, sizeof(url),
                 "https://firestore.googleapis.com/v1/projects/%s/databases/(default)/documents/%s/%s",
                 firebaseProjectId, collection, documentPath);
        http.begin(wClient, url);
        int httpResponseCode = http.PATCH(jsonDoc.as<String>());
        return httpResponseCode;
    }
};