#include "STR.h"
#include "Slave.h"
#include "esp_log_buffer.h"

static const char* tag = "STR";

std::array<std::reference_wrapper<ICore>, CORE_MAX> coreArray {

#if CONFIG_STR_ESPNOW
	ESPNowCore::GetInstance(),
#endif

#if CONFIG_STR_UART0
	UARTCore::GetUART0Instance(),
#endif

#if CONFIG_STR_UART1
	UARTCore::GetUART1Instance(),
#endif

#if CONFIG_STAR_UART0
	UARTCore::GetUART2Instance(),
#endif

};

std::array<Slave, CONFIG_STR_NB_SLAVE> slaveArray;
uint8_t nbInitialisedSlave = 0;

OptSlaveRef GetSlaveForID(const uint8_t id)
{
	for (auto& slave : slaveArray)
	{
		if (!slave)
			break;

		if (slave.GetID() == id || slave.HasSubSlave(id))
			return slave;
	}

	return std::nullopt;
}

OptSlaveRef GetSlaveForEMac(const std::array<uint8_t, 6> &eMac)
{
	LOG("Looking for slave with EMac");
	ESP_LOG_BUFFER_HEX(tag, eMac.data(), eMac.size());

	for (auto& slave : slaveArray)
	{
		if (!slave)
			break;

		if (eMac == slave.GetEMac())
			return slave;
	}

	return std::nullopt;
}

void		InitCores()
{
#if CONFIG_STR_ESPNOW
	ESPNowCore::GetInstance().Init();
#endif

#if CONFIG_STR_UART0
	UARTCore::GetUART0Instance().Init();
#endif

#if CONFIG_STR_UART1
	UARTCore::GetUART1Instance().Init();
#endif

#if CONFIG_STAR_UART0
	UARTCore::GetUART2Instance().Init();
#endif
}
