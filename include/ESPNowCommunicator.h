#pragma once

#include "Definitions.h"

#if STR_HAS_ESPNOW
#include "Communicator.h"
#include <cstdint>
#include <esp_now.h>
#include "ESPNowCore.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <array>
#include <initializer_list>
#include "Buffer.h"

class Message;

class ESPNowCTR: public ICTR
{
    public:

    ESPNowCTR(ESPNowCore& core, const std::array<uint8_t, 6>& mac, const bool createTimer = false);

    int         WriteImpl(Message& buf);
    void        UpdateImpl();

    void ConfigEspNowDirectNotif(const std::array<uint8_t, 6>& mac, uint8_t notifByte, uint8_t dstSlaveID);

    void ConfigEspNowDirectSettingUpdate(const std::array<uint8_t, 6>& mac, uint8_t settingRef, uint8_t settingValueLen, uint8_t dstSlaveID);
 
    void RemoveDirectNotifConfig(uint8_t dstSlaveID, uint8_t notifByte);

    void RemoveDirectSettingUpdateConfig(uint8_t dstSlaveID, uint8_t settingRef);

    void SendDirectNotif(uint8_t notifByte);

    void SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen);

	static ESPNowCTR*	GetCTRForMac(const std::array<uint8_t, 6>& mac);

	uint16_t	GetLinkInfoSizeImpl() const;

	void		WriteLinkInfoToBufferImpl(uint16_t index) const;

    void        SendPong() {
		uint32_t deltaMs = pdTICKS_TO_MS(xTaskGetTickCount()) - fLastMsgTimestamp;

		fCore.Write({
			Message::Frame::Start,
			0x00,
			0x0C,
			0,
			Message::Type::EspNowPong,
			(uint8_t)fLastMsgRssi,
			(uint8_t)fLastMsgNoiseFloor,
			(uint8_t)(deltaMs >> 24),
			(uint8_t)(deltaMs >> 16),
			(uint8_t)(deltaMs >> 8),
			(uint8_t)(deltaMs),
			Message::Frame::End
		}, fMac);
	}

    void        SendPing() {
		fCore.Write({
			Message::Frame::Start,
			0x00,
			0x06,
			0,
			Message::Type::EspNowPing,
			Message::Frame::End
		}, fMac);

		if (fPingTimer)
			xTimerChangePeriod(fPingTimer, pdMS_TO_TICKS(2000), 0);
	}

	int		Write(std::initializer_list<uint8_t> message) const {
		return fCore.Write(message, fMac);
	}

	int		Write() const {
		return fCore.Write(fMac);
	}

    const std::array<uint8_t, 6>&  GetMac() const { return fMac; }

    void            ShouldSendPing(bool should = true);

    private:

	ESPNowCTR() = delete;

	ESPNowCore&	fCore;

	std::array<uint8_t, 6>	fMac;

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

#endif
