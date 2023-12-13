#include "Message.h"
#include <Arduino.h>

Message::Message(uint8_t* buffer, uint8_t len) : fBuffer(buffer), fType((Message::Type)buffer[3]), fLength(len) {}

uint8_t Message::GetLength()
{
    return fLength;
}

byte* Message::GetBufPtr()
{
    return fBuffer;
}

void Message::ExtractSettingUpdate(uint8_t &ref, uint8_t &newValueLen, byte **newValue)
{
    Serial.println("Extracting Setting Update message");
    ref = 0;
    newValueLen = 0;
    *newValue = nullptr;

    if (fBuffer[3] == Message::Type::SettingUpdate)
    {
        Serial.println("ref");
        ref = fBuffer[4];
        Serial.println("value Len");
        newValueLen = fBuffer[5];
        Serial.println("value");
       *newValue = &fBuffer[6];
    }
    Serial.println("Done");
}