#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <TaskScheduler.h>
#include <time.h>

#include "wifi_service.h"
#include "flowmeter.h"
#include "models.h"
#include "kalman.h"

void taskReading();
void taskSending();

Task reading(1000, TASK_FOREVER, &taskReading);
Task sending(10000, TASK_FOREVER, &taskSending);

KalmanFilter kalman; // Create instance
PayloadDeviceName payloadDeviceName("Flowmeter-1");
WiFiClientSecure wClient;
WiFiService wifiService;
FlowMeter flowMeter;
Scheduler scheduler;
PayloadData payloadData;

// MQTT setup
WiFiClient mqttNetClient;
PubSubClient mqttClient(mqttNetClient);
const char *mqtt_broker = "103.150.117.46";
const uint16_t mqtt_port = 1883;
const char *mqtt_user = "arr1";
const char *mqtt_pass = "arr1";
const char *mqtt_client_id = "Flowmeter-Sani";
String lastPayloadData = "";
bool useKalmanFilter = true; // Toggle filtering

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
	String data;
	for (unsigned int i = 0; i < length; i++)
	{
		data += (char)payload[i];
	}
	if (data == "START")
		lastPayloadData = "START";
	else if (data == "STOP")
		lastPayloadData = "STOP";
}

void mqttReconnect()
{
	while (!mqttClient.connected())
	{
		Serial.print("Attempting MQTT connection...");
		if (mqttClient.connect(mqtt_client_id, mqtt_user, mqtt_pass))
		{
			Serial.println("connected");
			mqttClient.subscribe("your/topic");
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

void IRAM_ATTR ISR_function()
{
	flowMeter.incrementPulseCount();
}

void setup()
{
	pinMode(32, INPUT_PULLUP);
	Serial.begin(115200);
	wClient.setInsecure();
	wifiService.connect();

	mqttClient.setServer(mqtt_broker, mqtt_port);
	mqttClient.setCallback(mqttCallback);
	mqttReconnect();

	scheduler.init();
	scheduler.addTask(reading);
	reading.enable();

	attachInterrupt(digitalPinToInterrupt(32), ISR_function, RISING);
}

void loop()
{
	wifiService.reconnect();
	scheduler.execute();
	if (!mqttClient.connected())
	{
		mqttReconnect();
	}
	mqttClient.loop();
}

void taskReading()
{
	if (lastPayloadData == "START")
	{
		flowMeter.resetPulseCount();
		lastPayloadData = "";
	}

	if (lastPayloadData == "STOP")
	{
		HTTPClient http;

		// Get volume in mL
		float volume = flowMeter.getVolumeMilliLiters();
		float filteredVolume = useKalmanFilter ? kalman.filter(volume) : volume;

		// Get doc ID and set payload
		uint32_t currentId = wifiService.getDocument(http, wClient);
		payloadData.setLogId(currentId + 1);
		payloadData.setValue(volume); // Use mL value, cast to integer
		payloadData.setValueKalman(filteredVolume);
		flowMeter.resetPulseCount();

		taskSending();
		lastPayloadData = "";
	}
}

void taskSending()
{
	HTTPClient http;
	JsonDocument docData = payloadData.toJson();
	JsonDocument docName = payloadDeviceName.toJson();
	int responseCreate = wifiService.createDocument(http, wClient, docData);
	int responseUpdate = wifiService.updateDocument(http, wClient, docName);
}