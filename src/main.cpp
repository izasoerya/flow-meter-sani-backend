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

Task reading(15000, TASK_FOREVER, &taskReading);

KalmanFilter kalman;
PayloadDeviceName payloadDeviceName("Flowmeter-1");

WiFiClientSecure mqttSecureClient;
WiFiClientSecure httpSecureClient; // <== Tambahkan klien terpisah untuk HTTPS

WiFiService wifiService;
FlowMeter flowMeter;
Scheduler scheduler;
PayloadData payloadData;

// MQTT setup
PubSubClient mqttClient(mqttSecureClient); // <== Gunakan klien MQTT yang dedicated
const char *mqtt_broker = "m3f1b41a.ala.us-east-1.emqxsl.com";
const uint16_t mqtt_port = 8883;
const char *mqtt_user = "arr1";
const char *mqtt_pass = "arr1";
const char *mqtt_client_id = "Flowmeter-Sani";
bool useKalmanFilter = true;

bool isActive = false;
unsigned long flowStartTime = 0;

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
	String data;
	for (unsigned int i = 0; i < length; i++)
		data += (char)payload[i];

	Serial.println(data);

	if (data == "START")
	{
		isActive = true;
		flowStartTime = millis();
	}
	else if (data == "STOP")
	{
		isActive = false;
		flowMeter.resetPulseCount();
	}
}

void mqttReconnect()
{
	while (!mqttClient.connected())
	{
		Serial.print("Attempting MQTT connection...");
		if (mqttClient.connect(mqtt_client_id, mqtt_user, mqtt_pass))
		{
			Serial.println("connected");
			mqttClient.subscribe("Flowmeter-Sani/Flowmeter-1");
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

	mqttSecureClient.setInsecure(); // Tidak menggunakan validasi sertifikat
	httpSecureClient.setInsecure(); // Gunakan klien HTTPS terpisah

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
	if (isActive)
	{
		unsigned long now = millis();
		unsigned long durationMs = now - flowStartTime;

		float flowRateLPM = flowMeter.getFlowRateLPM(durationMs);
		float filteredFlow = kalman.filter(flowRateLPM);

		// Gunakan httpSecureClient saat melakukan HTTP requeste
		HTTPClient http;
		uint32_t currentId = wifiService.getDocument(http, httpSecureClient); // gunakan klien terpisah

		payloadData.setLogId(currentId + 1);
		payloadData.setValue(flowRateLPM * 3 * 0.782 * 1.14);
		payloadData.setValueKalman(filteredFlow * 3 * 0.782 * 1.14);

		JsonDocument docData = payloadData.toJson();
		JsonDocument docName = payloadDeviceName.toJson();

		int responseCreate = wifiService.createDocument(http, httpSecureClient, docData);
		int responseUpdate = wifiService.updateDocument(http, httpSecureClient, docName);

		Serial.print("Flow rate (L/min): ");
		Serial.println(flowRateLPM);

		flowMeter.resetPulseCount();
		flowStartTime = now;
	}
}
