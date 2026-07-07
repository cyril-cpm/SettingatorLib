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
    LORACTR() : fCore(LORACore::GetInstance()) {}

    void Update() {}

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
	}

    void SetPeerAddress(uint16_t address) {
        fPeerAddress = address;
        fActivated = true;
    }

    uint16_t GetPeerAddress() const { return fPeerAddress; }

    // TODO: ajouter LORA dans ICTR::LinkType pour remplacer UNKNOWN
    uint16_t GetLinkInfoSizeImpl() const { return 3; }

    void WriteLinkInfoToBufferImpl(uint16_t index) const {
        messageBuffer[index]     = static_cast<uint8_t>(ICTR::LinkType::UNKNOWN);
        messageBuffer[index + 1] = static_cast<uint8_t>(fPeerAddress >> 8);
        messageBuffer[index + 2] = static_cast<uint8_t>(fPeerAddress & 0xFF);
    }

	static OptLORACtrRef	GetCTRByAddress(std::initializer_list<uint8_t> address);

private:
    LORACore&   fCore;
    uint16_t    fPeerAddress = 0;
};


inline std::array<LORACTR, NB_LORA_CTR> loraCtrArray;
inline uint8_t registeredLoRaCtr;

#endif
