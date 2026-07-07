#include "Definitions.h"

#if STR_HAS_ESPNOW

#include "ESPNowCommunicator.h"

OptESPNowCtrRef ESPNowCTR::GetCTRByAddress(std::initializer_list<uint8_t> address)
{
	if (address.size() != 6)
	{
		ESP_LOGE("ESPNOWCTR", "address size %d instead of 6", address.size());
		return std::nullopt;
	}

	std::array<uint8_t, 6> mac;

	std::copy(address.begin(), address.end(), mac.data());
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
