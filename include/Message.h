#ifndef _MESSAGE_
#define _MESSAGE_

#include <Arduino.h>

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
    Message(uint8_t* buffer, uint8_t len);

    /*
    - Return length of buffer
    */
    uint8_t GetLength();

    /*
    - Return fBuffer ptr
    */
    byte*   GetBufPtr();

    /*
    - Return message type
    */
   Type GetType() const { return fType; }

private:
    byte*   fBuffer;
    Type    fType = Uninitialised;
    uint8_t fLength;
};

#endif