#pragma once

#include "Definitions.h"
#include <functional>
#include <initializer_list>
#include <optional>
#include "Core.h"
#include "Slave.h"
#include "freertos/idf_additions.h"

enum CoreEnum {
#if STR_HAS_ESPNOW
	CORE_ESPNOW,
#endif

#if STR_UART0
	CORE_UART0,
#endif

#if STR_UART1
	CORE_UART1,
#endif

#if STR_UART2
	CORE_UART2,
#endif

#if STR_HAS_LORA
	CORE_LORA,
#endif

	CORE_MAX
};

extern std::array<std::reference_wrapper<ICore>, CORE_MAX> coreArray;

#if CONFIG_STR_HAS_BRIDGE
extern std::array<Slave, CONFIG_STR_NB_SLAVE> slaveArray;
extern uint8_t nbInitialisedSlave;

OptSlaveRef	GetSlaveForID(const uint8_t id);
OptSlaveRef	GetSlaveForEMac(const std::array<uint8_t, 6>& eMac);
OptSlaveRef	CreateSlave(std::array<uint8_t, 6>&& eMac, uint8_t id = 0);
#endif

void		InitCores();
