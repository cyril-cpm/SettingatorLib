#pragma once

#include "Core.h"
#include "Definitions.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"

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
#if CONFIG_STR_SLAVE_ESPNOW
		HandleBridgeToSlaveHandshake();
		HandlePingTimerCreation();
		HandlePingSending();
#endif

#if CONFIG_STR_MASTER_ESPNOW
		HandleSlaveIDTransmission();
#endif
	}

	static ESPNowCTR*	GetCTRForMac(const std::array<uint8_t, 6>& mac);

#if CONFIG_STR_SLAVE_ESPNOW
	uint16_t	GetLinkInfoSizeImpl() const { return 19; };

	void		WriteLinkInfoToBufferImpl(uint16_t index) const {
		messageBuffer[index] = ICTR::LinkType::ESP_NOW;

		memcpy(messageBuffer.data() + index + 1, fMac.data(), 6);

		messageBuffer[index + 7] = atomic_load(fLastMsgRssi);
		messageBuffer[index + 8] = atomic_load(fLastMsgNoiseFloor);

		uint32_t deltaMs = pdTICKS_TO_MS(xTaskGetTickCount()) - atomic_load(fLastMsgTimestamp);

		memcpy(messageBuffer.data() + index + 9, (uint8_t*)&deltaMs, 4);

		messageBuffer[index + 13] = atomic_load(fPeerLastMsgRssi);
		messageBuffer[index + 14] = atomic_load(fPeerLastMsgNoiseFloor);

		memcpy(messageBuffer.data() + index + 15, (uint8_t*)&(atomic_load(fPeerLastMsgDeltastamp)), 4);
	}
#endif

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

#if CONFIG_STR_SLAVE_ESPNOW
		if (fPingTimer)
			xTimerChangePeriod(fPingTimer, pdMS_TO_TICKS(5000), 0);
#endif
	}

#if CONFIG_STR_MASTER_ESPNOW
	void			PlanifySlaveIDTransmission() {
		atomic_store(&fShouldTransmitSlaveID, true);
	}

	void			HandleSlaveIDTransmission() {
		if (atomic_exchange(&fShouldTransmitSlaveID, false))
		{
			uint8_t slaveID = Settingator::GetInstance().GetSlaveID();
			messageBuffer[0] =  SLAVEID_TRANSMISSION_TO_BRIDGE;
			ESP_ERROR_CHECK(esp_efuse_mac_get_default(messageBuffer.data() + 1));
			messageBuffer[7] = slaveID;
			messageBuffer.SetLen(8);
			Write();
			ESP_LOGI("ESPNowCTR", "Sending Slave ID to Bridge %d", slaveID);
		}
	}

	void			PlanifyPongSending() {
		atomic_store(&fShouldSendPong, true);
	}

	void			HandlePongSending() {
		if (std::atomic_exchange(&fShouldSendPong, false))
		{
			uint32_t deltaMs = pdTICKS_TO_MS(xTaskGetTickCount()) - fLastMsgTimestamp;

			Write({ LINK_PONG,
					(uint8_t)fLastMsgRssi,
					(uint8_t)fLastMsgNoiseFloor,
					(uint8_t)(deltaMs >> 24),
					(uint8_t)(deltaMs >> 16),
					(uint8_t)(deltaMs >> 8),
					(uint8_t)(deltaMs)
				});

		}
	}
#endif

#if CONFIG_STR_SLAVE_ESPNOW
	void			PlanifyBridgeToSlaveHandshake() {
		atomic_store(&fShouldHandshake, true);
	}

	void			HandleBridgeToSlaveHandshake() {
		if (atomic_exchange(&fShouldHandshake, false))
		{
			Write({ BRIDGE_TO_SLAVE_HANDSHAKE });
			ESP_LOGI("ESPNowCTR", "Handshake sent to slave");
		}
	}

	void			PlanifyPingTimerCreation() {
		atomic_store(&fShouldCreatePingTimer, true);
	}

	void			HandlePingTimerCreation() {
		if (atomic_exchange(&fShouldCreatePingTimer, false))
		{
			fPingTimer = xTimerCreate(
					"pingTimer",
					pdMS_TO_TICKS(5000),
					pdTRUE,
					(void*)this,
					[](TimerHandle_t timer) {
						ESPNowCTR* ctr = (ESPNowCTR*)pvTimerGetTimerID(timer);

						ctr->PlanifyPingSending();
					}
				);
		 	xTimerStart(fPingTimer, 0);
			ESP_LOGI("ESPNowCTR", "CREATING PING TIMER");
		}
	}

	void			PlanifyPingSending() {
		atomic_store(&fShouldSendPing, true);
	}

	void			HandlePingSending() {
		if (atomic_exchange(&fShouldSendPing, false))
			Write({ LINK_PING });
	}
#endif

    private:

	ESPNowCore&	fCore;

	std::array<uint8_t, 6>	fMac;

	std::atomic_uint32_t	fLastMsgTimestamp = 0;
	std::atomic_int8_t		fLastMsgRssi = 0;
	std::atomic_int8_t		fLastMsgNoiseFloor = 0;

#if CONFIG_STR_MASTER_ESPNOW
	std::atomic_bool	fShouldTransmitSlaveID = false;
	std::atomic_bool	fShouldSendPong = false;
#endif

#if CONFIG_STR_SLAVE_ESPNOW
	std::atomic_bool	fShouldSendPing = false;
	std::atomic_bool	fShouldHandshake = false;
	std::atomic_bool    fShouldCreatePingTimer = false;

    TimerHandle_t   fPingTimer = nullptr;

	std::atomic_uint32_t	fPeerLastMsgDeltastamp = 0;
	std::atomic_int8_t		fPeerLastMsgRssi = 0;
	std::atomic_int8_t		fPeerLastMsgNoiseFloor = 0;

#endif
};


inline bool compareMac(const uint8_t* mac1, const uint8_t* mac2) {
	return memcmp(mac1, mac2, 6) == 0;
}

inline std::array<ESPNowCTR, NB_ESPNOW_CTR> espNowCtrArray;
inline uint8_t registeredEspNowCtr;

inline std::optional<std::reference_wrapper<ESPNowCTR>> GetESPNowCommunicatorByMac(const std::array<uint8_t, 6>& mac)
{
	for (auto& ctr : espNowCtrArray)
	{
		if (!ctr)
			break;

		if (mac == ctr.GetMac())
			return ctr;
	}
	return std::nullopt;
}

#endif
