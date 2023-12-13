#include <Arduino.h>
#include "MiscDef.h"
#include "Settingator.h"
#include "Setting.h"
#include "WebSocketCommunicator.h"
#include "HTTPServer.h"
#include <FastLED.h>

#define DATA_PIN 19
#define NUM_LEDS 5

CRGB leds[NUM_LEDS];

WebSocketCTR* ctr;
STR*          settingator;
String str("Salut maggle");
String yolo("yolo");


uint8_t red = 127;
uint8_t green = 50;
uint8_t blue = 245;

HTTPServer *server;

void setup() {
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

    Serial.begin(9600);
    
    STR::StartWiFi();
    ctr = WebSocketCTR::CreateInstance(8081);
    server = new HTTPServer(8080);
    settingator = new STR(ctr);

    settingator->AddSetting(Setting::Type::Slider, &red, sizeof(red), "red");
    settingator->AddSetting(Setting::Type::Slider, &green, sizeof(green), "lawngreen");
    settingator->AddSetting(Setting::Type::Slider, &blue, sizeof(blue), "blue");
}

void loop() {
  // put your main code here, to run repeatedly:

  settingator->Update();
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = CRGB(red, green, blue);

  FastLED.show();
}