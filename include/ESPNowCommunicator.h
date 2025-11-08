#pragma once

#include "Communicator.h"
#include <cstdint>
#include <esp_now.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

class Message;

struct espNowMsg {
    espNowMsg(const uint8_t* inData, int inLen, uint32_t inTimestamp = 0, int8_t inNoiseFloor = 0, int8_t inRssi = 0);
    ~espNowMsg();

    uint8_t*    data = nullptr;
    int         len = 0;
    uint32_t    timestamp = 0;
    int8_t      noiseFloor = 0;
    int8_t      rssi = 0;
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
    const uint8_t*    GetMac() const;
    void    CreateLinkInfoTimer();
    void    HandleLinkInfo();
    void    shouldsendlinkinfo(bool should = true);

    static void         receiveCallback(const esp_now_recv_info* info, const uint8_t* data, int len);

    private:
    uint8_t fMac[6];
    TimerHandle_t       fLinkInfoTimer = nullptr;
    uint32_t            fEspNowVersion = 0;
    bool                fShouldSendLinkInfo = false;
};

extern ESPNowCore* espNowCore;

class ESPNowCTR: public ICTR
{
    public:
    static ESPNowCTR*   CreateInstanceWithMac(const uint8_t* mac, const bool createTimer = false);

    virtual int         Write(Message& buf) override;
    virtual void        Update() override;

    virtual void ConfigEspNowDirectNotif(uint8_t* mac, uint8_t notifByte, uint8_t dstSlaveID) override;

    virtual void ConfigEspNowDirectSettingUpdate(uint8_t* mac, uint8_t settingRef, uint8_t settingValueLen, uint8_t dstSlaveID) override;
 
    virtual void RemoveDirectNotifConfig(uint8_t dstSlaveID, uint8_t notifByte) override;

    virtual void RemoveDirectSettingUpdateConfig(uint8_t dstSlaveID, uint8_t settingRef) override;

    virtual void SendDirectNotif(uint8_t notifByte) override;

    virtual void SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen) override;
    
    static ESPNowCTR*   FindCTRForMac(const uint8_t* mac);

	virtual uint16_t	GetLinkInfoSize() const override;

	virtual void		WriteLinkInfo() const override;

    static void         HandleLinkInfo();

    void        SendPing();
    void        SendPong();

    const uint8_t*  GetMac() const;

    void            ShouldSendPing(bool should = true);

    private:
    ESPNowCTR(const uint8_t* mac = nullptr, const bool createTimer = false);
    ~ESPNowCTR();

    void _bufferizeMessage(espNowMsg* msg);

    uint8_t*    fMessageBuffer = nullptr;
    uint16_t    fMessageBufferSize = 0;

    uint8_t*    fMac = nullptr;

    std::vector<espNowDirectNotif*> fDirectNotif;
    std::vector<espNowDirectSettingUpdate*> fDirectSettingUpdate;

    static std::vector<ESPNowCTR*>          fCTRList;

    uint32_t    fLastMsgTimestamp = 0;
    int8_t      fLastMsgRssi = 0;
    int8_t      fLastMsgNoiseFloor = 0;

    uint32_t    fPeerLastMsgDeltastamp = 0;
    int8_t      fPeerLastMsgRssi = 0;
    int8_t      fPeerLastMsgNoiseFloor = 0;

    TimerHandle_t   fPingTimer = nullptr;
    
    ESPNowCore* fCore = nullptr;

    bool        fShouldSendPing = false;
};
