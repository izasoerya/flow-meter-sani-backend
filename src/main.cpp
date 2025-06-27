#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>

#include "wifi_service.h"
#include "flowmeter.h"
#include "models.h"
#include "kalman.h"

// -- Flowmeter & Kalman
FlowMeter flowMeter;
KalmanFilter kalman;
PayloadDeviceName payloadDeviceName("Flowmeter-1");

// -- Network
WiFiClientSecure mqttSecureClient;
WiFiClientSecure httpSecureClient;
PubSubClient mqttClient(mqttSecureClient);
WiFiService wifiService;

// -- MQTT Config
const char *mqtt_broker = "m3f1b41a.ala.us-east-1.emqxsl.com";
const uint16_t mqtt_port = 8883;
const char *mqtt_user = "arr1";
const char *mqtt_pass = "arr1";
const char *mqtt_client_id = "Flowmeter-Sani";

// -- System State
bool isActive = false;
unsigned long flowStartTime = 0;
uint32_t currentId = 0;

// -- RTOS Handles
TaskHandle_t TaskNetworkHandle;
TaskHandle_t TaskFlowmeterHandle;
QueueHandle_t flowQueue;

// -- ISR (Core 0)
void IRAM_ATTR ISR_function()
{
	flowMeter.incrementPulseCount();
}

// -- MQTT Callback
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

// -- MQTT Reconnect
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

// -- RTOS Task: Core 1 (MQTT + HTTP)
void TaskNetwork(void *parameter)
{
	PayloadData receivedPayload;

	for (;;)
	{
		wifiService.reconnect();

		if (!mqttClient.connected())
		{
			mqttReconnect();
		}
		mqttClient.loop();

		if (xQueueReceive(flowQueue, &receivedPayload, 10 / portTICK_PERIOD_MS))
		{
			// Send to HTTP
			JsonDocument docData = receivedPayload.toJson();
			JsonDocument docName = payloadDeviceName.toJson();

			HTTPClient http;
			wifiService.createDocument(http, httpSecureClient, docData);
			wifiService.updateDocument(http, httpSecureClient, docName);

			Serial.println("✅ HTTP sent from core 1");
		}

		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

// -- RTOS Task: Core 0 (Only Flow Calculation)
void TaskFlowmeter(void *parameter)
{
	for (;;)
	{
		if (isActive)
		{
			unsigned long now = millis();
			unsigned long durationMs = now - flowStartTime;

			// [Optional] Simulate pulses for testing
			// for (int i = 0; i < 100; ++i)
			// ISR_function();

			float flowRateLPM = flowMeter.getFlowRateLPM(durationMs);
			float filteredFlow = kalman.filter(flowRateLPM);

			if (filteredFlow > 200 || filteredFlow < 30)
			{
				kalman.reset();
			}

			currentId++;

			PayloadData payloadData;
			payloadData.setLogId(currentId);
			payloadData.setValue(flowRateLPM * 3 * 0.781 * 0.941);
			payloadData.setValueKalman(filteredFlow * 3 * 0.781 * 0.941);

			// Send to Core 1 via queue
			if (xQueueSend(flowQueue, &payloadData, 0) != pdPASS)
			{
				Serial.println("❌ Queue full, dropping data");
			}

			flowMeter.resetPulseCount();
			flowStartTime = now;
		}

		vTaskDelay(3000 / portTICK_PERIOD_MS); // every 10s
	}
}

void setup()
{
	pinMode(32, INPUT_PULLUP);
	Serial.begin(115200);

	mqttSecureClient.setInsecure(); // Skip TLS cert validation
	httpSecureClient.setInsecure();

	wifiService.connect();
	mqttClient.setServer(mqtt_broker, mqtt_port);
	mqttClient.setCallback(mqttCallback);
	mqttReconnect();

	attachInterrupt(digitalPinToInterrupt(32), ISR_function, RISING);

	HTTPClient http;
	currentId = wifiService.getDocument(http, httpSecureClient); // get initial logId

	// Create queue for inter-task communication
	flowQueue = xQueueCreate(5, sizeof(PayloadData));
	if (flowQueue == NULL)
	{
		Serial.println("❌ Failed to create queue");
		while (true)
			; // halt
	}

	// Create task for MQTT + HTTP (Core 1)
	xTaskCreatePinnedToCore(
		TaskNetwork,
		"TaskNetwork",
		8192,
		NULL,
		1,
		&TaskNetworkHandle,
		1);

	// Create task for Flowmeter Calculation (Core 0)
	xTaskCreatePinnedToCore(
		TaskFlowmeter,
		"TaskFlowmeter",
		8192,
		NULL,
		1,
		&TaskFlowmeterHandle,
		0);
}

void loop()
{
	// Nothing here. RTOS handles everything
	vTaskDelete(NULL);
}
