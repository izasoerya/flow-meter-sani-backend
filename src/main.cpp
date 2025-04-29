#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <TaskScheduler.h>
#include <time.h>

#include "wifi_service.h"
#include "flowmeter.h"
#include "models.h"

void taskReading();
void taskSending();

Task reading(1000, TASK_FOREVER, &taskReading);
Task sending(10000, TASK_FOREVER, &taskSending);

WiFiClientSecure wClient;
WiFiService wifiService;
FlowMeter flowMeter;
Scheduler scheduler;
PayloadData payloadData;
PayloadDeviceName payloadDeviceName("Flowmeter-1");

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7;
const int daylightOffset_sec = 3600;

void IRAM_ATTR ISR_function()
{
	flowMeter.incrementPulseCount();
}

void setup()
{
	Serial.begin(115200);
	wClient.setInsecure();
	wifiService.connect(wClient);

	scheduler.init();
	scheduler.addTask(reading);
	scheduler.addTask(sending);
	reading.enable();
	sending.enable();

	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	attachInterrupt(digitalPinToInterrupt(32), ISR_function, RISING); // Attach interrupt to pin 2
}

void loop()
{
	wifiService.reconnect();
	scheduler.execute();
}

void taskReading()
{
	time_t now = time(nullptr);
	struct tm *timeinfo = localtime(&now);
	char timestampStr[32];
	strftime(timestampStr, sizeof(timestampStr), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

	if (timeinfo->tm_year + 1900 >= 2024)
	{
		HTTPClient http;
		uint32_t currentId = wifiService.getDocument(http, wClient);
		payloadData.setLogId(currentId + 1);
		payloadData.setValue(flowMeter.getPulseCount());
		payloadData.setTimeStamp(String(timestampStr));
	}
	flowMeter.resetPulseCount();
}

void taskSending()
{
	HTTPClient http;
	JsonDocument docData = payloadData.toJson();
	JsonDocument docName = payloadDeviceName.toJson();
	int responseCreate = wifiService.createDocument(http, wClient, docData);
	int responseUpdate = wifiService.updateDocument(http, wClient, docName);
}