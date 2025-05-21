#pragma once

#include "Communicator.h"

class Message;

struct espNowMsg {
    espNowMsg(const uint8_t* inData, int inLen);
    ~espNowMsg();

    uint8_t*    data = nullptr;
    int         len = 0;
};

struct espNowDirectNotif {
    espNowDirectNotif(const uint8_t* inMac, uint8_t inNotifByte, uint8_t inDstSlaveID);
    ~espNowDirectNotif();

    uint8_t*    mac = nullptr;
    uint8_t     notifByte = 0;
    uint8_t     dstSlaveID = 0;
};

struct espNowDirectSettingUpdate {
    espNowDirectSettingUpdate(const uint8_t* inMac, uint8_t inSettingRef, uint8_t inDstSlaveID, uint8_t inValueLen);
    ~espNowDirectSettingUpdate();

    uint8_t*    mac = nullptr;
    uint8_t     settingRef = 0;
    uint8_t     dstSlaveID = 0;
    uint8_t     valueLen = 0;
};

class ESPNowCore
{
    public:
    static ESPNowCore*  CreateInstance();
    ESPNowCore();

    int     Write(Message& buf, uint8_t* dstMac);
    void    Update();
    void    AddPeer(uint8_t* peerMac);
    void    BroadcastPing();
};

extern ESPNowCore* espNowCore;

class ESPNowCTR: public ICTR
{
    public:
    static ESPNowCTR*   CreateInstanceWithMac(uint8_t* mac);

    virtual int         Write(Message& buf) override;
    virtual void        Update() override;

    virtual void ConfigEspNowDirectNotif(uint8_t* mac, uint8_t notifByte, uint8_t dstSlaveID) override;

    virtual void ConfigEspNowDirectSettingUpdate(uint8_t* mac, uint8_t settingRef, uint8_t settingValueLen, uint8_t dstSlaveID) override;
 
    virtual void RemoveDirectNotifConfig(uint8_t dstSlaveID, uint8_t notifByte) override;

    virtual void RemoveDirectSettingUpdateConfig(uint8_t dstSlaveID, uint8_t settingRef) override;

    virtual void SendDirectNotif(uint8_t notifByte) override;

    virtual void SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen) override;
    
    private:
    ESPNowCTR(uint8_t* mac = nullptr);
    ~ESPNowCTR();

    uint8_t*    fMac = nullptr;

    std::vector<espNowDirectNotif*> fDirectNotif;
    std::vector<espNowDirectSettingUpdate*> fDirectSettingUpdate;

    ESPNowCore* fCore = nullptr;
};