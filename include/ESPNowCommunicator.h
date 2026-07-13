#pragma once

#include "Definitions.h"

#if STR_HAS_ESPNOW
#include "Communicator.h"
#include "ESPNowCore.h"
#include "Buffer.h"
#include "Link.h"

#include <atomic>
#include <cstdint>
#include <esp_now.h>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <array>
#include <initializer_list>
#include <optional>
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"

#if CONFIG_STR_MASTER_ESPNOW
#include "Settingator.h"
#endif

class Message;

using OptESPNowCtrRef = std::optional<std::reference_wrapper<ESPNowCTR>>;

class ESPNowCTR
	:
		public ICTR, 
		public CTRLink
#if CONFIG_STR_MASTER_ESPNOW
		,public CTRMasterLink
#endif

#if CONFIG_STR_SLAVE_ESPNOW
		,public CTRSlaveLink
#endif

{
    public:

#if CONFIG_STR_SLAVE_ESPNOW
	static constexpr LinkType linkType = LinkType::ESP_NOW;
#endif

	ESPNowCTR() : fCore(ESPNowCore::GetInstance()) {}

    void        Update() {
#if CONFIG_STR_SLAVE_ESPNOW
		CTRSlaveLink::Update();
#endif

#if CONFIG_STR_MASTER_ESPNOW
		CTRMasterLink::Update();
#endif
	}

	static ESPNowCTR*	GetCTRForMac(const std::array<uint8_t, 6>& mac);

	int		Write(std::initializer_list<uint8_t> message) const {
		return fCore.Write(message, fMac);
	}

	int		Write() const {
		return fCore.Write(fMac);
	}

	bool	HasThisAddress(std::initializer_list<uint8_t> address) const {
		if (address.size() != 6)
		{
			ESP_LOGE(
					"ESPNOWCTR",
					"SetAddressImpl: address size is %d instead of 6",
					address.size()
			);
			return false;
		}

		std::array<uint8_t, 6> addr;
		std::copy(address.begin(), address.end()-1, addr.data());

		return addr == fMac;
	}

	void	SetAddressImpl(std::initializer_list<uint8_t> address) {
		if (address.size() != 6)
			ESP_LOGE(
					"ESPNOWCTR",
					"SetAddressImpl: address size is %d instead of 6",
					address.size()
			);
		std::copy(address.begin(), address.end(), fMac.data());
		fActivated = true;
		fCore.AddPeer(fMac);
	}

	void	SetMac(std::array<uint8_t, 6>&& mac) {
		fMac = mac;
		fActivated = true;
		fCore.AddPeer(fMac);
	}

    const std::array<uint8_t, 6>&  GetMac() const { return fMac; }

	static OptESPNowCtrRef GetCTRByAddress(std::initializer_list<uint8_t> address);

    private:

	ESPNowCore&	fCore;

	std::array<uint8_t, 6>	fMac;
};



inline bool compareMac(const uint8_t* mac1, const uint8_t* mac2) {
	return memcmp(mac1, mac2, 6) == 0;
}

inline std::array<ESPNowCTR, NB_ESPNOW_CTR> espNowCtrArray;
inline uint8_t registeredEspNowCtr;

#endif
