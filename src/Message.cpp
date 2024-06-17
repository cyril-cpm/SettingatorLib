#include "Message.h"
#include <Arduino.h>
#include "MiscDef.h"

Message* Message::CreateMessageAdoptBuffer(uint8_t* buffer, uint8_t len)
{
    return new Message(buffer, len);
}

Message* Message::CreateMessageCopyBuffer(uint8_t* buffer, uint8_t len)
{
    uint8_t* copyBuffer = (byte*)malloc(len * sizeof(byte));
    memcpy(copyBuffer, buffer, len);

    return new Message(copyBuffer, len);
}

Message* Message::BuildInitRequestMessage(uint8_t slaveID)
{
    uint8_t buffer[] = {Message::Frame::Start,
                        0x00,
                        0x07,
                        slaveID,
                        Message::Type::InitRequest,
                        0,
                        Message::Frame::End};
    return new Message(buffer, 7);
}

Message::Message(uint8_t* buffer, uint8_t len) : fBuffer(buffer), fSlaveID(buffer[3]),
                                                    fType((Message::Type)buffer[4]), fLength(len)
{}

Message::~Message()
{
    delete fBuffer;
}

uint8_t Message::GetLength()
{
    return fLength;
}

uint8_t* Message::GetBufPtr()
{
    return fBuffer;
}

void Message::ExtractSettingUpdate(uint8_t &ref, uint8_t &newValueLen, byte **newValue)
{
    //Serial.println("Extracting Setting Update message");
    ref = 0;
    newValueLen = 0;
    *newValue = nullptr;

    if (fType == Message::Type::SettingUpdate)
    {
        DEBUG_PRINT_VALUE("SUCCESS: buffer type", fBuffer[4])
        ref = fBuffer[5];
        DEBUG_PRINT_VALUE("ref", ref)
        newValueLen = fBuffer[6];
        DEBUG_PRINT_VALUE("value len", newValueLen)
        *newValue = &fBuffer[7];
        DEBUG_PRINT_VALUE_BUF_LN("value", *newValue, newValueLen)
    }
    else
    {
        DEBUG_PRINT_VALUE("FAIL: buffer type", fBuffer[4])
    }
    //Serial.println("Done");
}

char* Message::ExtractSSD()
{
    return (char*)(fBuffer + 5);
}