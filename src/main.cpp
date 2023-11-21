#include <Arduino.h>
#include "MiscDef.h"
#include "Settingator.h"
#include "Setting.h"
#include "WebSocketCommunicator.h"
#include <HardwareSerial.h>
#include <FastLED.h>
#include <ESPAsyncWebServer.h>

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

const char HTML[] PROGMEM = "<textarea id=\"log\" name=\"log\" rows=\"20\" cols=\"120\"></textarea>\n<input type=\"range\" min=\"1\" max=\"100\" value=\"50\" class=\"slider\" id=\"myRange\">\n<script>\n\n    let log = document.getElementById(\"log\");\n\n    function logText(text) {\n        log.textContent += text + \"\\n\";\n    }\n    \n    function logHex(buff) {\n        for (i = 0; i < buff.length; i++) {\n            if (buff[i] < 16)\n                log.textContent += 0;\n            log.textContent += buff[i].toString(16);\n            log.textContent += \" \";\n        }\n        log.textContent += \"\\n\";\n    }\n\n    webSocket = new WebSocket(\"ws://192.168.0.1:8081/settingator\", \"settingator\");\n    webSocket.binaryType = \"arraybuffer\";\n\n    function sendInitRequest() {\n        let initRequest = new Uint8Array([ 0xFF, 0x00, 0x06, 0x12, 0x00, 0x00]);\n        webSocket.send(initRequest);\n        logText(\"Init request sent...\");\n        logHex(initRequest);\n    }\n\n    function treatMessage(message) {\n        logText(\"treating message\");\n        let view = new Uint8Array(message);\n        logHex(message);\n        logHex(view);\n    }\n\n    webSocket.onopen = function (event) {\n        logText(\"onOpen\");\n\n        sendInitRequest();\n    }\n\n    webSocket.onclose = function (event) {\n        logText(\"onClose \" + event.code + \" \" + event.reason);\n    }\n\n    webSocket.onmessage = function (event) {\n        logText(\"onMessage\");\n\n        treatMessage(event.data);\n    }\n\n    webSocket.onerror = function (event) {\n        logText(\"onError\");\n    }\n    \n\n\n</script>";

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

    server.on("/*", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("http request received");
      Serial.println(request->url());
      Serial.println(request->host());
      Serial.println(request->contentType());
      request->send(200, "text/html", HTML);
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