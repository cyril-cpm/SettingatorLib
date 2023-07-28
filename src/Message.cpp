#include "Message.h"

Message::Message(uint8_t* buffer, uint8_t len) : fBuffer(buffer), fType((Message::Type)buffer[3]), fLength(len) {}

uint8_t Message::GetLength()
{
    return fLength;
}

byte* Message::GetBufPtr()
{
    return fBuffer;
}