#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <PubSubClient.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "NotoSansBold15.h"
#include "CalibriBold12.h"
#include "CalibriBold16.h"
#include "CalibriBold18.h"

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
const char *ssid = "ipfire";
const char *password = "joseffranz";

// config mqtt
const char *mqtt_server = "192.168.123.105";

TFT_eSPI tft = TFT_eSPI();
#define AA_FONT_SMALL NotoSansBold15
#define AA_FONT_1 CalibriBold12
#define AA_FONT_2 CalibriBold16
#define AA_FONT_3 CalibriBold18

#define ST7735_BLACK 0x0000 /*   0,   0,   0 */
#define ST7735_WHITE 0xFFFF /* 255, 255, 255 */

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

AsyncWebServer server(80);

unsigned long last;

typedef struct
{
  String key;
  String value;
} KeyValue;

KeyValue keys[5];

KeyValue ParseKeyValue(String s)
{
  KeyValue kv;
  if(s.length() < 3) 
    return kv;

  int i = s.indexOf('=');
  if (i == -1)
    return kv;

  kv.key = s.substring(0, i);
  kv.value = s.substring(i + 1);
  return kv;
}

// To test & debug
void ParseMessage(String s, KeyValue kvs[])
{
  int l = s.length();
  int n = 0;
  int i = 0;
  int j = s.indexOf('\n');
  while (j != -1)
  {
    String row = s.substring(i,j);
    kvs[n++] = ParseKeyValue(row);
    i = j;
    j = s.indexOf('\n', i);
  }
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
    tft.setCursor(0, 8);
    tft.setTextPadding(20);
    tft.print(msg);
    // tft.drawRightString(msg, 50, 239, 1);
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
    if (mqttClient.connect("ESP8266_mqtt_tft"))
    {
      Serial.println("connected");
      mqttClient.subscribe("tft/text");
      mqttClient.subscribe("tft/backlight");
      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(5, 8);
      tft.print("MQTT connected");
      delay(3000);
      tft.fillScreen(ST7735_BLACK);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(5, 8);
      tft.print("MQTT lost !");
      delay(3000);
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
  tft.print("ESP8266_MQTT4");

  // wifi
  setupWifi();

  // mqtt
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callbackMqtt);

  // Update OTA
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hi! I am ESP8266."); });
  AsyncElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");

  analogWrite(Dimmer, 128);
  delay(500);
  Serial.println("Setup finished, start loop");
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