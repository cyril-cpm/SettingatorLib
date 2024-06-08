#include "Communicator.h"

#include "MiscDef.h"
#include "Message.h"

void ICTR::_receive(Message* msg)
{
    Serial.println("WebSocketCTR _receive");
    printBuffer(msg->GetBufPtr(), msg->GetLength(), HEX);
    Serial.println("");
    fReceivedMessage.push(msg);
}

uint8_t ICTR::GetBoxSize() const
{
    return fReceivedMessage.size();
}


bool ICTR::Available()
{
    Update();
    return fReceivedMessage.size();
}

Message* ICTR::Read()
{
    if (GetBoxSize())
        return fReceivedMessage.front();
    return nullptr;
}

void ICTR::Flush()
{
   delete fReceivedMessage.front();
   fReceivedMessage.pop();
}