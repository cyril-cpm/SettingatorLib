#include "WebSocketCommunicator.h"
#include <WiFi.h>
#include "MiscDef.h"

WebSocketCTR::WebSocketCTR(int port)
{   
    fWebSocketServer = new WebSocketsServer(port, "/settingator", "settingator");
    fWebSocketServer->onEvent([this](uint8_t client, WStype_t type, uint8_t* payload, size_t len) {
        Serial.println("WebSocketServerEvent");
        printBuffer(payload, len);
        Serial.println("");
        Serial.println(type);
        switch (type)
        {
            case (WStype_BIN):
            {
                _receive(new Message(payload, len));
                break;
            }
        }
    });
    Serial.println("begin websocketserver");
    fWebSocketServer->begin();
    Serial.println("finish");
}

WebSocketCTR* WebSocketCTR::CreateInstance(int port)
{
    return new WebSocketCTR(port);
}

bool WebSocketCTR::Available()
{
    Update();
    return fReceivedMessage.size();
}

Message* WebSocketCTR::Read()
{
   return fReceivedMessage.front();
}

void WebSocketCTR::Flush()
{
   delete fReceivedMessage.front();
   fReceivedMessage.pop();
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

void WebSocketCTR::_receive(Message* msg)
{
    Serial.println("WebSocketCTR _receive");
    printBuffer(msg->GetBufPtr(), msg->GetLength(), HEX);
    Serial.println("");
    fReceivedMessage.push(msg);
}