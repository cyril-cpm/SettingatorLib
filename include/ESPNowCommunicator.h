#pragma once

#include "Definitions.h"

#if STR_HAS_ESPNOW
#include "Communicator.h"
#include <atomic>
#include <cstdint>
#include <esp_now.h>
#include "ESPNowCore.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <array>
#include <initializer_list>
#include "Buffer.h"
#include <optional>

#if CONFIG_STR_MASTER_ESPNOW
#include "Settingator.h"
#endif

class Message;

class ESPNowCTR: public ICTR
{
    public:

	ESPNowCTR() : fCore(ESPNowCore::GetInstance()) {}

    ESPNowCTR(ESPNowCore& core, const std::array<uint8_t, 6>& mac, const bool createTimer = false);

    void        Update() {
		HandleSlaveIDTransmission();
	}

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

	void	SetMac(std::array<uint8_t, 6>& mac) {
		fMac = mac;
		fActivated = true;
		fCore.AddPeer(fMac);
	}

    const std::array<uint8_t, 6>&  GetMac() const { return fMac; }

	void			SetLinkInfo(uint8_t rssi, uint8_t noiseFloor, uint32_t timestamp) {
		fLastMsgRssi = rssi;
		fLastMsgNoiseFloor = noiseFloor;
		fLastMsgTimestamp = timestamp;
	}

	void			PlanifySlaveIDTransmission() {
		atomic_store(&fShouldTransmitSlaveID, true);
	}

	void			HandleSlaveIDTransmission() {
		if (atomic_exchange(&fShouldTransmitSlaveID, false))
		{
			messageBuffer[0] =  SLAVEID_TRANSMISSION;
			ESP_ERROR_CHECK(esp_efuse_mac_get_default(messageBuffer.data() + 1));
#if CONFIG_STR_MASTER_ESPNOW
			messageBuffer[7] = Settingator::GetInstance().GetSlaveID();
#endif
			messageBuffer.SetLen(8);
			Write();
		}
	}

    private:

	ESPNowCore&	fCore;

	std::array<uint8_t, 6>	fMac;

    uint32_t    fLastMsgTimestamp = 0;
    int8_t      fLastMsgRssi = 0;
    int8_t      fLastMsgNoiseFloor = 0;

    uint32_t    fPeerLastMsgDeltastamp = 0;
    int8_t      fPeerLastMsgRssi = 0;
    int8_t      fPeerLastMsgNoiseFloor = 0;

    TimerHandle_t   fPingTimer = nullptr;

	std::atomic_bool	fShouldSendPing = false;

	std::atomic_bool	fShouldTransmitSlaveID = false;
};

bool compareMac(const uint8_t* mac1, const uint8_t* mac2);

extern std::array<ESPNowCTR, NB_ESPNOW_CTR> espNowCtrArray;
extern uint8_t registeredEspNowCtr;

std::optional<std::reference_wrapper<ESPNowCTR>> GetESPNowCommunicatorByMac(const std::array<uint8_t, 6>& mac);
#endif
