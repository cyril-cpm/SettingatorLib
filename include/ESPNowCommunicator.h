#pragma once

#include "Communicator.h"
#include <cstdint>
#include <esp_now.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <array>

class Message;

struct espNowMsg {
    espNowMsg(const uint8_t* inData, int inLen, uint32_t inTimestamp = 0, int8_t inNoiseFloor = 0, int8_t inRssi = 0);

	std::vector<uint8_t>	data;
    int						len = 0;
    uint32_t				timestamp = 0;
    int8_t					noiseFloor = 0;
    int8_t					rssi = 0;
};

struct espNowDirectNotif {
    espNowDirectNotif(const std::array<uint8_t, 6>& inMac, uint8_t inNotifByte, uint8_t inDstSlaveID);

	std::array<uint8_t, 6>    mac;
    uint8_t     notifByte = 0;
    uint8_t     dstSlaveID = 0;
};

struct espNowDirectSettingUpdate {
    espNowDirectSettingUpdate(const std::array<uint8_t, 6>& inMac, uint8_t inSettingRef, uint8_t inDstSlaveID, uint8_t inValueLen);

	std::array<uint8_t, 6>    mac;
    uint8_t     settingRef = 0;
    uint8_t     dstSlaveID = 0;
    uint8_t     valueLen = 0;
};

class ESPNowCore
{
    public:
    static ESPNowCore&  GetInstance();

    int     Write(Message&& buf, const std::array<uint8_t, 6>& dstMac);
    void    Update();
    void    AddPeer(const std::array<uint8_t, 6>& peerMac);
    void    BroadcastPing();
    const std::array<uint8_t, 6>&    GetMac() const;
    void    CreateLinkInfoTimer();
    void    HandleLinkInfo();
    void    shouldsendlinkinfo(bool should = true);

    static void         receiveCallback(const esp_now_recv_info* info, const uint8_t* data, int len);

    private:
    ESPNowCore();

	std::array<uint8_t, 6>	fMac;
    TimerHandle_t			fLinkInfoTimer = nullptr;
    uint32_t				fEspNowVersion = 0;
};

class ESPNowCTR: public ICTR
{
    public:

    static ESPNowCTR   CreateInstanceWithMac(const std::array<uint8_t, 6>& mac, const bool createTimer = false);

    int         WriteImpl(Message& buf);
    void        UpdateImpl();

    void ConfigEspNowDirectNotif(const std::array<uint8_t, 6>& mac, uint8_t notifByte, uint8_t dstSlaveID);

    void ConfigEspNowDirectSettingUpdate(const std::array<uint8_t, 6>& mac, uint8_t settingRef, uint8_t settingValueLen, uint8_t dstSlaveID);
 
    void RemoveDirectNotifConfig(uint8_t dstSlaveID, uint8_t notifByte);

    void RemoveDirectSettingUpdateConfig(uint8_t dstSlaveID, uint8_t settingRef);

    void SendDirectNotif(uint8_t notifByte);

    void SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen);

	static ESPNowCTR*	GetCTRForMac(const std::array<uint8_t, 6>& mac);

	uint16_t	GetLinkInfoSize() const;

	void		WriteLinkInfoToBuffer(uint8_t* buffer) const;

    void        SendPing();
    void        SendPong();

    const std::array<uint8_t, 6>&  GetMac() const;

    void            ShouldSendPing(bool should = true);

    private:
    ESPNowCTR(const std::array<uint8_t, 6>& mac, const bool createTimer = false);

	ESPNowCTR() = default;

	std::array<uint8_t, 6>	fMac;

    std::vector<espNowDirectNotif> fDirectNotif;
    std::vector<espNowDirectSettingUpdate> fDirectSettingUpdate;

    static std::vector<ESPNowCTR*>          fCTRList;

    uint32_t    fLastMsgTimestamp = 0;
    int8_t      fLastMsgRssi = 0;
    int8_t      fLastMsgNoiseFloor = 0;

    uint32_t    fPeerLastMsgDeltastamp = 0;
    int8_t      fPeerLastMsgRssi = 0;
    int8_t      fPeerLastMsgNoiseFloor = 0;

    TimerHandle_t   fPingTimer = nullptr;

    bool        fShouldSendPing = false;
};

bool compareMac(const uint8_t* mac1, const uint8_t* mac2);
