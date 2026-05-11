#pragma once

#include "Definitions.h"

#if STR_HAS_ESPNOW
#include "esp_err.h"
#include <esp_mac.h>
#include "Buffer.h"
#include "Core.h"
#include <esp_now.h>

class ESPNowCore : public ICore
{
	public:
	
		static ESPNowCore& GetInstance()
		{
			static ESPNowCore instance;
			return instance;
		}

		void Init();

		int	 Write(std::initializer_list<uint8_t>message,
				const std::array<uint8_t, 6>& dstMac) {
			ESP_ERROR_CHECK_WITHOUT_ABORT(esp_now_send(dstMac.data(), message.begin(), message.size()));
			return 0;
		}

		int	Write(const std::array<uint8_t, 6>& dstMac) {
			ESP_LOGI("ESPNowCore", "sending");
			ESP_ERROR_CHECK_WITHOUT_ABORT(esp_now_send(dstMac.data(), messageBuffer.data(), messageBuffer.len()));
			ESP_LOGI("ESPNowCore", "done");
			return 0;
		}

		void	AddPeer(const std::array<uint8_t, 6>& peerMac);

#if CONFIG_STR_MASTER_ESPNOW
		void	BroadcastSlavePing() {
			std::array<uint8_t, 6> dstMac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

			messageBuffer[0] = SLAVE_BROADCAST_PING;
			ESP_ERROR_CHECK(esp_efuse_mac_get_default(messageBuffer.data() + 1));
			messageBuffer.SetLen(7);
			
			AddPeer(dstMac);

			Write(dstMac);

			ESP_LOGI("ESPNowCore", "BroadcastSlavePing");
		}

#endif

#if CONFIG_STR_SLAVE_ESPNOW
		void	BroadcastBridgePing() {
			std::array<uint8_t, 6> dstMac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

			messageBuffer[0] = BRIDGE_BROADCAST_PING;
			ESP_ERROR_CHECK(esp_efuse_mac_get_default(messageBuffer.data() + 1));
			messageBuffer.SetLen(7);

			AddPeer(dstMac);

			Write(dstMac);

			ESP_LOGI("ESPNowCore", "BroadcastBridgePing");
		}
#endif

		const std::array<uint8_t, 6>&	 GetMac() const {return fMac; }
		void	CreateLinkInfoTimer();
		void	HandleLinkInfo();
		void	shouldsendlinkinfo(bool should = true);

		static void			receiveCallback(const esp_now_recv_info* info, const uint8_t* data, int len);

	private:

		std::array<uint8_t, 6>	fMac;
};
#endif
