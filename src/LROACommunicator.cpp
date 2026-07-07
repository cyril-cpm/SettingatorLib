#include "Definitions.h"

#if STR_HAS_LORA

#include "LORACommunicator.h"

OptLORACtrRef LORACTR::GetCTRByAddress(std::initializer_list<uint8_t> address)
{
	if (address.size() != 2)
	{
		ESP_LOGE("LORACTR", "address size %d instead of 2", address.size());
		return std::nullopt;
	}

	uint16_t addr = (*address.begin() << 8) + *(address.end() - 1);

    for (auto& ctr : loraCtrArray)
    {
        if (!ctr)
            break;
        if (ctr.GetPeerAddress() == addr)
            return ctr;
    }
    return std::nullopt;
}

#endif
