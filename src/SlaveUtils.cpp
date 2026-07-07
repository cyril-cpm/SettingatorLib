#include "Definitions.h"

#if CONFIG_STR_HAS_BRIDGE

#include "Slave.h"
#include "SlaveUtils.h"
#include "MiscDef.h"
#include "Link.h"
#include "LORACommunicator.h"
#include "ESPNowCommunicator.h"

static const char* tag = "SLAVE_UTILS";

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

void		InitSlaveCTR(
		OptSlaveRef slave,
		std::initializer_list<uint8_t> srcAddress,
		uint8_t ctrIndex
	)
{
}

#endif
