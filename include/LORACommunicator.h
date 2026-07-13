#pragma once

#include "Definitions.h"

#if STR_HAS_LORA

#include "Communicator.h"
#include "LORACore.h"
#include "Link.h"

#include <array>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include "esp_types.h"


using OptLORACtrRef = std::optional<std::reference_wrapper<LORACTR>>;

class LORACTR 
	: 
		public ICTR, 
		public CTRLink
#if CONFIG_STR_MASTER_LORA
		,public CTRMasterLink
#endif

#if CONFIG_STR_SLAVE_LORA
		,public CTRSlaveLink
#endif
{
public:

#if CONFIG_STR_SLAVE_LORA
	static constexpr LinkType linkType = LinkType::LORA;
#endif

    LORACTR() : fCore(LORACore::GetInstance()) {}

    int Write(std::initializer_list<uint8_t> message) const {
        return fCore.Write(fPeerAddress, fCore.GetChannel(), message);
    }

    int Write() const {
        return fCore.Write(fPeerAddress, fCore.GetChannel());
    }

	bool	HasThisAddress(std::initializer_list<uint8_t> address) const {
		if (address.size() != 2)
		{
			ESP_LOGE(
					"LORACTR",
					"SetAddressImpl: address size is %d instead of 2",
					address.size()
			);
			return false;
		}

		uint16_t addr = (*address.begin() << 8) + *(address.end() - 1);
		return addr == fPeerAddress;
	}

	void	SetAddressImpl(std::initializer_list<uint8_t> address) {
		if (address.size() != 2)
			ESP_LOGE(
					"LORACTR",
					"SetAddressImpl: address size is %d instead of 2",
					address.size()
			);
		
		fPeerAddress = (*address.begin() << 8) + *(address.end() - 1);
		fActivated = true;
	}

    void SetPeerAddress(uint16_t address) {
        fPeerAddress = address;
        fActivated = true;
    }

    uint16_t GetPeerAddress() const { return fPeerAddress; }

	void	Update() {
#if STR_SLAVE_HAS_LORA
		CTRSlaveLink::Update();
#endif

#if STR_MASTER_HAS_LORA
		CTRMasterLink::Update();
#endif
	}

	static OptLORACtrRef	GetCTRByAddress(std::initializer_list<uint8_t> address);

private:
    LORACore&   fCore;
    uint16_t    fPeerAddress = 0;
};


inline std::array<LORACTR, NB_LORA_CTR> loraCtrArray;
inline uint8_t registeredLoRaCtr = 0;

#endif
