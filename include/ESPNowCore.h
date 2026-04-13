#pragma once

#include "Definitions.h"

#if STR_HAS_ESPNOW
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

		ESPNowCore();
		void Init();

		int	 Write(std::initializer_list<uint8_t>message,
				const std::array<uint8_t, 6>& dstMac) {
			esp_now_send(dstMac.data(), message.begin(), message.size());
			return 0;
		}

		int	Write(const std::array<uint8_t, 6>& dstMac) {
			esp_now_send(dstMac.data(), messageBuffer.data(), messageBuffer.len());
			return 0;
		}

		void	Update();
		void	AddPeer(const std::array<uint8_t, 6>& peerMac);
		void	BroadcastPing();
		const std::array<uint8_t, 6>&	 GetMac() const {return fMac; }
		void	CreateLinkInfoTimer();
		void	HandleLinkInfo();
		void	shouldsendlinkinfo(bool should = true);

		static void			receiveCallback(const esp_now_recv_info* info, const uint8_t* data, int len);

	private:

		std::array<uint8_t, 6>	fMac;
};
#endif
