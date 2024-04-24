#ifndef _MESSAGE_
#define _MESSAGE_

#include <sys/_stdint.h>

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
    Message(uint8_t** buffer, uint8_t len);

    /*
    - Return length of buffer
    */
    uint8_t GetLength();

    /*
    - Return fBuffer ptr
    */
    uint8_t*   GetBufPtr();

    /*
    - Return message type
    */
   Type GetType() const { return fType; }

    /*
    - Get Setting Update Message
    - Get 0 or null if wrong message type
    */
   void ExtractSettingUpdate(uint8_t &ref, uint8_t &newValueLen, uint8_t **newValue);

private:
    uint8_t*   fBuffer;
    Type    fType = Uninitialised;
    uint8_t fLength;
};

#endif