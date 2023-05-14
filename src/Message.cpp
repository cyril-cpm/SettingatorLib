#include "Message.h"
#include "Buffer.h"

Message::Message(byte* buffer) : fBuffer(buffer), fType((Message::Type)buffer[3]) {}

uint8_t Message::GetLength()
{
    return fLength;
}

byte* Message::GetBufPtr()
{
    return fBuffer;
}