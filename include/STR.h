#pragma once

#include "Definitions.h"
#include <functional>
#include <optional>
#include "Core.h"
#include "Slave.h"
#include "Master.h"

#if STR_HAS_ESPNOW
#include "ESPNowCore.h"
#endif

#if STR_HAS_UART
#include "UARTCore.h"
#endif

enum CoreEnum {
#if CONFIG_STR_ESPNOW
	CORE_ESPNOW,
#endif

#if CONFIG_STR_UART0
	CORE_UART0,
#endif

#if CONFIG_STR_UART1
	CORE_UART1,
#endif

#if CONFIG_STR_UART2
	CORE_UART2,
#endif

	CORE_MAX
};

extern std::array<std::reference_wrapper<ICore>, CORE_MAX> coreArray;

#if CONFIG_STR_HAS_BRIDGE
extern std::array<Slave, CONFIG_STR_NB_SLAVE> slaveArray;
extern uint8_t nbInitialisedSlave;

OptSlaveRef	GetSlaveForID(const uint8_t id);
OptSlaveRef	GetSlaveForEMac(const std::array<uint8_t, 6>& eMac);
#endif

void		InitCores();
