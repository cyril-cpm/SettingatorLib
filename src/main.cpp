#include <Arduino.h>
#include "MiscDef.h"
#include "Settingator.h"
#include "Setting.h"
#include "WebSocketCommunicator.h"
#include <HardwareSerial.h>
#include <FastLED.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

//ICTR* communicator = WebSocketCTR::CreateInstance();
//STR settingator = STR(communicator);

#define DATA_PIN 2
#define NUM_LEDS 5

CRGB leds[NUM_LEDS];

AsyncWebServer server(8080);
WebSocketCTR* ctr;
STR*          settingator;
String str("Salut maggle");
String yolo("yolo");



void setup() {
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

    Serial.begin(9600);
    Serial.println("\nBegin test:");

    
    uint8_t value = 8;
    Setting test(Setting::Type::Slider, str.begin(), str.length(), "yolo", 4);
    value = 42;

    size_t size = 0;
    byte* buffer = nullptr;

    size = test.getInitRequestSize();

    buffer = (byte*)malloc(size * sizeof(buffer));
    test.getInitRequest(buffer);

    Serial.print("Returned size : ");
    Serial.println(size);

    if (buffer)
    {
        Serial.print("Buffer is : ");
        printBuffer(buffer, size, HEX);
    }
    else
        Serial.println("Buffer is nullptr");

    Serial.println("");
    printBuffer((byte*)"yolo", 4, HEX);
    Serial.println("");

    Serial.println("Begin WiFi");
    WiFi.mode(WIFI_AP);

    WiFi.softAPConfig(IPAddress(192,168,0,1), IPAddress(192,168,0,1), IPAddress(255,255,255,0));
    WiFi.softAP("espTest", "123456788");
    Serial.println(WiFi.softAPIP());

    if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
      }
    server.on("/*", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("http request received");
      Serial.println(request->url());
      Serial.println(request->host());
      Serial.println(request->contentType());
      request->send(SPIFFS, "/index.html", "text/html");
    });

    Serial.println("Begin Web Server");
    server.begin();
    Serial.println("finish");

    
    ctr = WebSocketCTR::CreateInstance(8081);
    settingator = new STR(ctr);
    settingator->AddSetting(test);
}

void loop() {
  // put your main code here, to run repeatedly:

  settingator->Update();
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = CRGB::Red;

  FastLED.show();
}