#include "WebSocketCommunicator.h"

WebSocketCTR::WebSocketCTR(int port)
{
    fWebSocketServer = new WebSocketsServer(port, "", "settingator");
    fWebSocketServer->onEvent([this](uint8_t client, WStype_t type, uint8_t* payload, size_t len) {
        switch (type)
        {
            case (WStype_BIN):
            {
                _receive(new Message(payload, len));
                break;
            }
        }
    });
    fWebSocketServer->begin();
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

void WebSocketCTR::Flush(Message& message)
{
   delete fReceivedMessage.front();
   fReceivedMessage.pop();
}

int WebSocketCTR::Write(Message& msg)
{
    
}

void WebSocketCTR::Update()
{
   fWebSocketServer->loop();
}

void WebSocketCTR::_receive(Message* msg)
{
    fReceivedMessage.push(msg);
}