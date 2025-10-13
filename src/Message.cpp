#include "Message.h"
#include "MiscDef.h"

#if defined(ARDUINO)
#include <Arduino.h>

#elif defined(ESP_PLATFORM)
#include <stdlib.h>
#include <esp_types.h>
#include <cstring>

#endif

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

Message* Message::BuildSlaveIDRequestMessage()
{
    uint8_t buffer[] = {
        Message::Frame::Start,
        0x00,
        0x06,
        0,
        Message::Type::SlaveIDRequest,
        Message::Frame::End
    };

    return new Message(buffer, 6);
}

Message* Message::BuildReInitSlaveMessage()
{
    uint8_t buffer[] = {
        Message::Frame::Start,
        0x00,
        0x06,
        0,
        Message::Type::BridgeReinitSlaves,
        Message::Frame::End
    };

    return new Message(buffer, 6);
}

Message::Message(uint8_t* buffer, uint16_t len)
{
    fSlaveID = buffer[3];
    fType = (Message::Type)buffer[4];
    fLength = len;
    fBuffer = (uint8_t*)malloc(len * sizeof(uint8_t));
    memcpy(fBuffer, buffer, len);
}

Message::Message(uint8_t** buffer, uint16_t len)
{
    fSlaveID = *buffer[3];
    fType = (Message::Type)(*buffer)[4];
    fLength = len;
    fBuffer = *buffer;
}

Message::~Message()
{
    delete fBuffer;
}

uint16_t Message::GetLength()
{
    return fLength;
}

uint8_t* Message::GetBufPtr()
{
    return fBuffer;
}

uint16_t Message::ExtractSettingUpdate(uint8_t &ref, uint8_t &newValueLen, uint8_t **newValue, uint16_t settingIndex)
{
    //Serial.println("Extracting Setting Update message");
    ref = 0;
    newValueLen = 0;
    *newValue = nullptr;

    if (fType == Message::Type::SettingUpdate)
    {
        ref = fBuffer[settingIndex];
        DEBUG_PRINT_VALUE("ref", ref)
        newValueLen = fBuffer[settingIndex + 1];
        DEBUG_PRINT_VALUE("value len", newValueLen)
        *newValue = &fBuffer[settingIndex + 2];
        DEBUG_PRINT_VALUE_BUF_LN("value", *newValue, newValueLen)
        return settingIndex + 2 + newValueLen;

    }
    else
    {
        DEBUG_PRINT_VALUE("FAIL: buffer type", fBuffer[4])
    }
    //Serial.println("Done");
    return (fLength - 1);
}

char* Message::ExtractSSD()
{
    return (char*)(fBuffer + 5);
}

uint8_t Message::operator[](uint16_t index)
{
    if (index >= fLength)
        index = fLength - 1;

    return fBuffer[index];
}