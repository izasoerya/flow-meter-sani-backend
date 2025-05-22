#pragma once

#include <PubSubClient.h>
#include <WiFiClient.h>

class MQTTService
{
private:
    const char *broker = "103.150.117.46";
    const uint16_t port = 1883;
    PubSubClient mqttClient;
    WiFiClient client;
    static String _lastPayloadData;

    static void parseMessage(char *topic, byte *payload, unsigned int length)
    {
        String data;
        for (int i = 0; i < length; i++)
        {
            data += (char)payload[i];
        }
        if (data == "START")
            _lastPayloadData = "START";
        else if (data == "STOP")
            _lastPayloadData = "STOP";
    }

public:
    MQTTService() : mqttClient(PubSubClient(client)) {}

    ~MQTTService() {}

    void begin()
    {
        mqttClient.setServer(broker, port);
        mqttClient.connect("Flowmeter-Sani");
    }

    void loop()
    {
        mqttClient.setCallback(parseMessage);
    }

    String getPayload()
    {
        return _lastPayloadData;
    }

    void reconnect()
    {
        while (!mqttClient.connected())
        {
            Serial.print("Attempting MQTT connection...");
            // Set username and password to "arr1"
            if (mqttClient.connect("Flowmeter-Sani"))
            {
                Serial.println("connected");
            }
            else
            {
                Serial.print("failed, rc=");
                Serial.print(mqttClient.state());
                Serial.println(" try again in 5 seconds");
                delay(5000);
            }
        }
    }
};

String MQTTService::_lastPayloadData = "";