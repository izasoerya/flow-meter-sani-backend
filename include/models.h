#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class PayloadData
{
private:
    uint32_t logId;
    float value;
    float kalmanValue;

public:
    PayloadData() {};
    ~PayloadData() {};

    void setLogId(const uint32_t &logId)
    {
        this->logId = logId;
    }
    void setValue(const float &value)
    {
        this->value = value;
    }
    void setValueKalman(const float &value)
    {
        this->kalmanValue = value;
    }

    JsonDocument toJson()
    {
        JsonDocument doc; // Adjust size as needed
        JsonObject fields = doc.createNestedObject("fields");

        JsonObject logIdObj = fields.createNestedObject("logId");
        logIdObj["integerValue"] = String(logId); // Firestore expects stringified integers

        JsonObject valueObj = fields.createNestedObject("value");
        valueObj["doubleValue"] = String(value);

        JsonObject valueKalmanObj = fields.createNestedObject("kalmanValue");
        valueKalmanObj["doubleValue"] = String(kalmanValue);

        return doc;
    }
};

class PayloadDeviceName
{
private:
    String deviceName;

public:
    PayloadDeviceName(const String &deviceName) : deviceName(deviceName) {};
    ~PayloadDeviceName() {};

    JsonDocument toJson()
    {
        JsonDocument doc;
        JsonObject fields = doc.createNestedObject("fields");

        JsonObject deviceNameObj = fields.createNestedObject("deviceName");
        deviceNameObj["stringValue"] = deviceName;

        return doc;
    }
};