#ifndef _MESSAGE_
#define _MESSAGE_

#include <sys/_stdint.h>

class Message
{
public:
    enum Type
    {
        Uninitialised,

        /// Settingator
        InitRequest = 0x12,
        SettingUpdate = 0x11,
        SettingInit = 0x13,
        Notif = 0x14,

        /// Bridge
        BridgeBase = 0x50,
        EspNowInitWithSSD = 0x54
    };

    enum Frame
    {
        Start = 0xFF,
        End = 0x00
    };

    static Message* CreateMessageAdoptBuffer(uint8_t* buffer, uint8_t len);
    static Message* CreateMessageCopyBuffer(uint8_t* buffer, uint8_t len);

    static Message* BuildInitRequestMessage(uint8_t slaveID);

    ~Message();

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

   uint8_t GetSlaveID() const { return fSlaveID; }

    /*
    - Get Setting Update Message
    - Get 0 or null if wrong message type
    */
   void ExtractSettingUpdate(uint8_t &ref, uint8_t &newValueLen, uint8_t **newValue);

   char*    ExtractSSD();

private:
    Message() {};
    Message(uint8_t* buffer, uint8_t len);

    uint8_t*   fBuffer;
    Type    fType = Uninitialised;
    uint8_t fSlaveID = 0;
    uint8_t fLength;
};

#endif