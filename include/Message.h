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
        ConfigEspNowDirectNotif = 0x15,
        ConfigEspNowDirectSettingUpdate = 0x16,
        RemoveDirectNotifConfig = 0x17,
        RemoveDirectSettingUpdateConfig = 0x18,
        BroadcastedPing = 0x19,

        /// Bridge
        BridgeBase = 0x50,
        EspNowInitWithSSD = 0x54,
        EspNowConfigDirectNotif = 0x55,
        EspNowConfigDirectSettingUpdate = 0x56,
        EspNowRemoveDirectNotifConfig = 0x57,
        EspNowRemoveDirectSettingUpdateConfig = 0x58,
        EspNowStartInitBroadcastedSlave = 0x59,
        EspNowStopInitBroadcastedSlave = 0x5A,
        BridgeReinitSlaves = 0x5B,
        SlaveIDRequest = 0x5C
    };

    enum Frame
    {
        Start = 0xFF,
        End = 0x00
    };

    static Message* BuildInitRequestMessage(uint8_t slaveID);
    static Message* BuildSlaveIDRequestMessage();

    Message() {};
    Message(uint8_t* buffer, uint8_t len);
    Message(uint8_t** buffer, uint8_t len);
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
    uint8_t*   fBuffer;
    Type    fType = Uninitialised;
    uint8_t fSlaveID = 0;
    uint8_t fLength;
};

#endif