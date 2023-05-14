#ifndef _MESSAGE_
#define _MESSAGE_

#include "Buffer.h"

class Message
{
public:
    enum Type
    {
        Uninitialised,
        InitRequest = 0x12,
        SettingUpdate = 0x11,
        SettingInit = 0x13
    };

    enum Frame
    {
        Start = 0xFF,
        End = 0x00
    };

    Message() {};
    Message(byte* buffer);

    /*
    - Return length of buffer
    */
    uint8_t GetLength();

    /*
    - Return fBuffer ptr
    */
    byte*   GetBufPtr();

private:
    byte*   fBuffer;
    Type    fType = Uninitialised;
    uint8_t fLength;
};

#endif