#include "WebSocketCommunicator.h"

#include "MiscDef.h"
#include "Message.h"

#include <WebSocketsServer.h>

WebSocketCTR::WebSocketCTR(int port)
{   
    fWebSocketServer = new WebSocketsServer(port, "/settingator", "settingator");

#if SERIAL_DEBUG
    Serial.println("initializing WebSocketCTR");
#endif

    fWebSocketServer->onEvent([this](uint8_t client, WStype_t type, uint8_t* payload, size_t len) {

#if SERIAL_DEBUG
        Serial.println("//////////////////////////////////////////////////////////////////////////");
        Serial.println("WebSocketServerEvent");
        printBuffer(payload, len);
        Serial.println("");
        Serial.println(type);
        Serial.println("onEvent");
#endif

        switch (type)
        {
            case (WStype_BIN):
            {
                _receive(new Message(payload, len));
                break;
            }
        }
    });

#if SERIAL_DEBUG
    Serial.println("begin websocketserver");
#endif
    fWebSocketServer->begin();
#if SERIAL_DEBUG
    Serial.println("finish");
#endif
}

WebSocketCTR* WebSocketCTR::CreateInstance(int port)
{

#if 0
    DEBUG_PRINT("Wifi status : ")
    DEBUG_PRINT_LN(WiFi.status())

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(WiFi.mode(WIFI_STA));
        WiFi.begin("Freebox-8DF141", "tinnire86-exsurdat@8-inrepat.-peragitur3");
        Serial.print("Connecting to WiFi ..");
        while (WiFi.status() != WL_CONNECTED)
        {
            Serial.print('.');
            delay(1000);
        }
    }
#endif
    return new WebSocketCTR(port);
}


int WebSocketCTR::Write(Message& msg)
{
    Serial.println("broadcasting Buffer:");
    printBuffer(msg.GetBufPtr(), msg.GetLength());
    Serial.println("");
    fWebSocketServer->broadcastBIN(msg.GetBufPtr(), msg.GetLength());
    return 0;
}

void WebSocketCTR::Update()
{
   fWebSocketServer->loop();
}