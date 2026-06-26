// #include <Arduino.h>
//  #include <ESPWiFi.h>
//  #include <ESPAsyncTCP.h>
//  #include <ESPAsyncWebServer.h>
// #include <WiFiClient.h>
// #include <ESP8266WebServer.h>
// #include <SPI.h>
// #include "NotoSansBold15.h"

#include <ESP8266WiFi.h>
#include <ElegantOTA.h>
#include <PubSubClient.h>
#include <TFT_eSPI.h>
#include "CalibriBold12.h"
#include "CalibriBold16.h"
#include "CalibriBold18.h"
#include "secrets.h"

// ST7735 TFT module 128x160 connections
// 1 - GND
// 2 - VCC
// 3 - SCK  D5
// 4 - SDA  D7
// 5 - RST  D4
// 6 - DC   D3
// 7 - CS   D2

// config IO
#define LED D0
#define Dimmer D1

// config wifi
// see secrets.h for ssid and password

// config mqtt
//const char *mqtt_server = "192.168.123.105";
IPAddress mqtt_server(192, 168, 123, 105);


TFT_eSPI tft = TFT_eSPI();
// #define AA_FONT_SMALL NotoSansBold15
#define AA_FONT_1 CalibriBold12
#define AA_FONT_2 CalibriBold16
#define AA_FONT_3 CalibriBold18

#define ST7735_BLACK 0x0000 /*   0,   0,   0 */
#define ST7735_WHITE 0xFFFF /* 255, 255, 255 */

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

AsyncWebServer server(80);

unsigned long last;

int splitString(const String &src, char sep, String parts[], int maxParts)
{
	int count = 0;
	int start = 0;
	int idx = src.indexOf(sep, start);

	while (idx != -1 && count < maxParts - 1)
	{
		parts[count++] = src.substring(start, idx);
		start = idx + 1;
		idx = src.indexOf(sep, start);
	}

	// Last part
	if (start <= src.length() && count < maxParts)
	{
		parts[count++] = src.substring(start);
	}

	return count;
}

void setupWifi()
{
	delay(10);
	Serial.print("Connecting to ");
	Serial.println(ssid);
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
	tft.setCursor(20, 60);
	tft.print("ip address:");
	tft.setCursor(20, 80);
	tft.print(WiFi.localIP());
}

void callbackMqtt(String topic, byte *payload, unsigned int length)
{
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print(",");
	Serial.print(length);
	Serial.println("] ");

	char cpay[length];
	for (unsigned int i = 0; i < length; i++)
	{
		cpay[i] = (char)payload[i];
	}
	cpay[length] = 0;
	String msg = String(cpay);
	Serial.println(msg);

	
	if (topic == "tft/text")
	{
		String parts[10];
		int numParts = splitString(msg, ';', parts, 16);
		for (int i = 0; i < numParts; i++)
		{
			Serial.print("Part ");
			Serial.print(i);
			Serial.print(": ");
			Serial.println(parts[i]);
		}

		if(numParts == 10)
		{
			tft.setTextColor(ST7735_WHITE, ST7735_BLACK, true);

			tft.setTextDatum(BL_DATUM);   // Top Left
			tft.loadFont(AA_FONT_1);
			tft.drawString(parts[0], 1, 24);
			tft.drawString(parts[2], 1, 56);
			tft.drawString(parts[4], 1, 88);
			tft.drawString(parts[6], 1, 120);
			tft.drawString(parts[8], 1, 152);

			tft.setTextDatum(BR_DATUM);   // Top Right
			tft.loadFont(AA_FONT_3);
			tft.drawString(parts[1], 127, 24);
			tft.drawString(parts[3], 127, 56);
			tft.drawString(parts[5], 127, 88);
			tft.drawString(parts[7], 127, 120);
			tft.drawString(parts[9], 127, 152);
		}

		// tft.setCursor(0, 8);
		// tft.setTextPadding(20);
		// tft.print(msg);
	}
	if (topic == "tft/backlight")
	{
		unsigned int i = String(msg).toInt();
		analogWrite(Dimmer, i);
	}
}

void reconnectMqtt()
{
	while (!mqttClient.connected())
	{
		Serial.print("Attempting MQTT connection...");

		if (mqttClient.connect("ESP8266_tft")) {
			Serial.println("connected");
			// mqttClient.publish("outTopic","hello world");
			mqttClient.subscribe("tft/text");
		} else {
			Serial.print("failed, rc=");
			Serial.print(mqttClient.state());
			Serial.println(" try again in 5 seconds");
			delay(5000);
		}
	}
}

void setup()
{
	// IO
	pinMode(LED, OUTPUT);
	pinMode(Dimmer, OUTPUT);
	digitalWrite(Dimmer, HIGH);
	// analogWrite(Dimmer, 128);

	// serial
	Serial.begin(115200);

	// tft
	delay(500);
	tft.begin();
	delay(100);
	tft.setRotation(2);
	tft.fillScreen(ST7735_BLACK);
	tft.setTextColor(ST7735_WHITE, ST7735_BLACK, true);
	tft.loadFont(AA_FONT_2);
	tft.setCursor(5, 20);
	tft.print("ESP8266_MQTT5");
	
	// wifi
	setupWifi();

	// mqtt
	mqttClient.setServer(mqtt_server, 1883);
	mqttClient.setCallback(callbackMqtt);

	// Update OTA
	server.on("/", [](AsyncWebServerRequest *request)
			  { request->send(200, "text/plain", "Hi! I am ESP8266."); });

	server.on("/reboot", [](AsyncWebServerRequest *request)
			  { request->send(200, "text/plain", "Rebooting...");
      delay(3000);
      ESP.restart(); });

	ElegantOTA.begin(&server); // Start ElegantOTA
	server.begin();
	Serial.println("HTTP server started");

	analogWrite(Dimmer, 128);
	delay(5000);
	Serial.println("Setup finished, start loop");
	tft.fillScreen(ST7735_BLACK);
}

void loop()
{
	if (!mqttClient.connected())
	{
		reconnectMqtt();
	}
	mqttClient.loop();

	if ((millis() - last) > 5000)
	{
		last = millis();
		digitalWrite(LED, HIGH);
		delay(50);
		digitalWrite(LED, LOW);
	}
	delay(100);
}