#include "STR.h"
#include "Slave.h"
#include "esp_log_buffer.h"
#include "freertos/idf_additions.h"

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

#if CONFIG_STR_HAS_BRIDGE
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

OptSlaveRef CreateSlave(std::array<uint8_t, 6> &&eMac, uint8_t id)
{
	ESP_LOGI("STR", "CreateSlave");
	if (nbInitialisedSlave < CONFIG_STR_NB_SLAVE)
	{
		ESP_LOGI("STR", "There is room dfor a slave");
		Slave& slave = slaveArray[nbInitialisedSlave];
		slave.SetEMac(std::move(eMac));
		slave.SetID(id);
		slave.Activate();
		nbInitialisedSlave++;

		if (!id)
			slave.PlanifySlaveIDRequest();

		return slave;
	}
	else
		ESP_LOGI("STR", "To much slave initialised: %d", nbInitialisedSlave);
	return std::nullopt;
}
#endif

void		InitCores()
{
	if (!mainTaskHandle)
		mainTaskHandle = xTaskGetCurrentTaskHandle();

#if CONFIG_STR_ESPNOW
	ESPNowCore::GetInstance().Init();
#endif

#if CONFIG_STR_UART0
	UARTCore::GetUART0Instance().Init();
#endif

#if CONFIG_STR_UART1
	UARTCore::GetUART1Instance().Init();
#endif

#if CONFIG_STR_UART2
	UARTCore::GetUART2Instance().Init();
#endif

#if STR_HAS_LORA
	LORACore::GetInstance().Init();
#endif
}
