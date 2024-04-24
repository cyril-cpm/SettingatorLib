#include "Message.h"
#include <Arduino.h>
#include "MiscDef.h"

Message::Message(uint8_t* buffer, uint8_t len) : fType((Message::Type)buffer[3]), fLength(len)
{
    fBuffer = (byte*)malloc(len * sizeof(byte));
    memcpy(fBuffer, buffer, len);
}

Message::Message(uint8_t** buffer, uint8_t len) : fType((Message::Type)(*buffer)[3]), fLength(len)
{
    fBuffer = *buffer;
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
        DEBUG_PRINT_VALUE("SUCCESS: buffer type", fBuffer[3])
        ref = fBuffer[4];
        DEBUG_PRINT_VALUE("ref", ref)
        newValueLen = fBuffer[5];
        DEBUG_PRINT_VALUE("value len", newValueLen)
       *newValue = &fBuffer[6];
        DEBUG_PRINT_VALUE_BUF_LN("value", *newValue, newValueLen)
    }
    else
    {
        DEBUG_PRINT_VALUE("FAIL: buffer type", fBuffer[3])
    }
    //Serial.println("Done");
}