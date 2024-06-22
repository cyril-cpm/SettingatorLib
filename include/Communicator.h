#ifndef _COMMUNICATOR_
#define _COMMUNICATOR_

#include <sys/_stdint.h>
#include <queue>

class Message;

class ICTR
{
    public:

    /*
    - return true if there is bytes available to read
    */
    virtual bool Available();

    /*
    - Write Buffer to communicator
    */
    virtual int Write(Message& buf) = 0;

    /*
    - Read a message if avaible or return empty Message
     */
    virtual Message* Read();

    /*
    - Flush message after having executed
    */
    virtual void    Flush();

    /*
    - Update internal Buffer
    */
    virtual void Update() = 0;

    virtual uint8_t GetBoxSize() const;

    virtual void ConfigEspNowDirectNotif(uint8_t* mac, uint8_t notifByte, uint8_t dstSlaveID);

    virtual void ConfigEspNowDirectSettingUpdate(uint8_t* mac, uint8_t settingRef, uint8_t settingValueLen, uint8_t dstSlaveID);

    virtual void SendDirectNotif(uint8_t notifByte);
    
    virtual void SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen);

    virtual void RemoveDirectNotifConfig(uint8_t dstSlaveID, uint8_t notifByte);

    virtual void RemoveDirectSettingUpdateConfig(uint8_t dstSlaveID, uint8_t settingRef);

    protected:

    void _receive(Message* msg);

    std::queue<Message*> fReceivedMessage;

};

#endif