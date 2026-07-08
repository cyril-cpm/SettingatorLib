#pragma once

#include "Definitions.h"

#if STR_HAS_LORA || STR_HAS_ESPNOW
#include "Settingator.h"
#include "Buffer.h"
#include "SlaveUtils.h"

#include "esp_types.h"
#include "esp_mac.h"
#include <atomic>
#include <optional>
#include <functional>
#include <array>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#define SLAVE_BROADCAST_PING 0x01
#define BRIDGE_BROADCAST_PING 0x02
#define LINK_PING 0x03
#define LINK_PONG 0x04
#define SLAVEID_TRANSMISSION_TO_BRIDGE 0x05
#define BRIDGE_TO_SLAVE_HANDSHAKE 0x06

class CTRLink
{
	friend class CTRMasterLink;

	public:

		void			SetLinkInfo(int8_t rssi, int8_t noiseFloor, uint32_t timestamp) {
			fLastMsgRssi = rssi;
			fLastMsgNoiseFloor = noiseFloor;
			fLastMsgTimestamp = timestamp;
		}

		void			SetAddress(
				this auto&& self,
				std::initializer_list<uint8_t> address
			) {
			self.SetAddressImpl(address);
		}


	protected:

		std::atomic_uint32_t	fLastMsgTimestamp = 0;
		std::atomic_int8_t		fLastMsgRssi = 0;
		std::atomic_int8_t		fLastMsgNoiseFloor = 0;
};

#if CONFIG_STR_HAS_SETTINGATOR

class CTRMasterLink
{
	public:

		void			PlanifySlaveIDTransmission() {
			atomic_store(&fShouldTransmitSlaveID, true);
		}

		void			HandleSlaveIDTransmission(this auto&& self) {
			if (atomic_exchange(&self.fShouldTransmitSlaveID, false))
			{
				uint8_t slaveID = Settingator::GetInstance().GetSlaveID();
				messageBuffer[0] =  SLAVEID_TRANSMISSION_TO_BRIDGE;
				ESP_ERROR_CHECK(esp_efuse_mac_get_default(messageBuffer.data() + 1));
				messageBuffer[7] = slaveID;
				messageBuffer.SetLen(8);
				self.Write();
				ESP_LOGI("ESPNowCTR", "Sending Slave ID to Bridge %d", slaveID);
			}
		}

		void			PlanifyPongSending() {
			atomic_store(&fShouldSendPong, true);
		}

		void			HandlePongSending(this auto&& self) {
			if (std::atomic_exchange(&self.fShouldSendPong, false))
			{
				uint32_t deltaMs =
						pdTICKS_TO_MS(xTaskGetTickCount())
						- self.fLastMsgTimestamp;

				ESP_LOGD("ESPNowCTR", "Sending Pong");

				self.Write({ LINK_PONG,
						(uint8_t)self.fLastMsgRssi,
						(uint8_t)self.fLastMsgNoiseFloor,
						(uint8_t)(deltaMs >> 24),
						(uint8_t)(deltaMs >> 16),
						(uint8_t)(deltaMs >> 8),
						(uint8_t)(deltaMs)
					});

			}
		}

	protected:

		std::atomic_bool	fShouldTransmitSlaveID = false;
		std::atomic_bool	fShouldSendPong = false;
};

#endif

#if CONFIG_STR_HAS_BRIDGE

class CTRSlaveLink
{
	public:

		void			PlanifyBridgeToSlaveHandshake() {
			atomic_store(&fShouldHandshake, true);
		}

		void			HandleBridgeToSlaveHandshake(this auto&& self) {
			if (atomic_exchange(&self.fShouldHandshake, false))
			{
				self.Write({ BRIDGE_TO_SLAVE_HANDSHAKE });
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
							CTRSlaveLink* ctr = (CTRSlaveLink*)pvTimerGetTimerID(timer);

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

		void			HandlePingSending(this auto&& self) {
			if (atomic_exchange(&self.fShouldSendPing, false))
			{
				ESP_LOGD("ESPNowCTR", "Ping Sending");
				self.Write({ LINK_PING });
				ESP_LOGD("ESPNowCTR", "Ping Sent");
			}
		}

	void		SetPeerLinkInfo(int8_t rssi, int8_t noiseFloor, uint32_t deltastamp) {
		fPeerLastMsgRssi = rssi;
		fPeerLastMsgNoiseFloor = noiseFloor;
		fPeerLastMsgDeltastamp = deltastamp;
	}

	protected:

		std::atomic_uint32_t	fPeerLastMsgDeltastamp = 0;
		std::atomic_int8_t		fPeerLastMsgRssi = 0;
		std::atomic_int8_t		fPeerLastMsgNoiseFloor = 0;

		std::atomic_bool	fShouldSendPing = false;
		std::atomic_bool	fShouldHandshake = false;
		std::atomic_bool    fShouldCreatePingTimer = false;

		TimerHandle_t   fPingTimer = nullptr;


};

inline bool initLinkBroadcasted = false;

#endif

#endif
