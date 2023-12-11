#include <Arduino.h>
#include "MiscDef.h"
#include "Settingator.h"
#include "Setting.h"
#include "WebSocketCommunicator.h"
#include "HTTPServer.h"
#include <FastLED.h>

#define DATA_PIN 2
#define NUM_LEDS 5

CRGB leds[NUM_LEDS];

WebSocketCTR* ctr;
STR*          settingator;
String str("Salut maggle");
String yolo("yolo");

u8_t val = 31;
char *test = "AAAAAAA";
u8_t b = 0;
u16_t big = 256;

HTTPServer *server;

void setup() {
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

    Serial.begin(9600);
    
    STR::StartWiFi();
    ctr = WebSocketCTR::CreateInstance(8081);
    server = new HTTPServer(8080);
    settingator = new STR(ctr);

    settingator->AddSetting(Setting::Type::Slider, &val, sizeof(u8_t));
    settingator->AddSetting(Setting::Type::Switch, &b, sizeof(b), test);
    settingator->AddSetting(Setting::Type::Slider, &big, sizeof(big));
}

void loop() {
  // put your main code here, to run repeatedly:

  settingator->Update();
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = CRGB::Red;

  FastLED.show();
}