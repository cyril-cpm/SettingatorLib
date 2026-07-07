#pragma once

#include "Definitions.h"

#if CONFIG_STR_HAS_BRIDGE


#include <functional>
#include <optional>
#include "esp_types.h"

class Slave;

using OptSlaveRef = std::optional<std::reference_wrapper<Slave>>;

OptSlaveRef GetSlaveForEMac(const std::array<uint8_t, 6> &eMac);
OptSlaveRef	CreateSlave(std::array<uint8_t, 6>&& eMac, uint8_t id = 0);
OptSlaveRef	GetSlaveForID(const uint8_t id);

void		InitSlaveCTR(
		OptSlaveRef slave,
		std::initializer_list<uint8_t> srcAddress,
		uint8_t ctrIndex
	);

#endif
