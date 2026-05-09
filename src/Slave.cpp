#include "Definitions.h"

#if CONFIG_STR_HAS_BRIDGE
#include "Slave.h"
#include "Communicator.h"
#include "ESPNowCommunicator.h"
#include "sdkconfig.h"
#include <variant>

bool initEspNowBroadcasted = false;

uint8_t Slave::GetID()
{
    return fSlaveID;
}

#if CONFIG_STR_NB_SUBSLAVE
void Slave::AddSubSlave(uint8_t id)
{
	if (fSubSlaveCount <= CONFIG_STR_NB_SUBSLAVE)
	{
		fSubSlave[fSubSlaveCount] = id;
		fSubSlaveCount++;
	}
}
#endif

void Slave::SetID(uint8_t id)
{
    fSlaveID = id;
}

uint16_t Slave::GetLinkInfoSize() const
{
	const auto& ctrToUse = fCTRArray[fCTRToUse];

	if (ctrToUse)
	{
		return std::visit([](auto&& ctr) -> uint16_t {
				return ctr.get().GetLinkInfoSize() + 2;
			}, ctrToUse);
	}

	return 0;
}

void Slave::WriteLinkInfoToBuffer(uint16_t index) const
{
	const auto& ctrToUse = fCTRArray[fCTRToUse];

	if (ctrToUse)
	{
		messageBuffer[index] = GetLinkInfoSize();
		messageBuffer[index + 1] = fSlaveID;

		std::visit([index](auto&& ctr) {

				ctr.get().WriteLinkInfoToBuffer(index + 2);
			
			}, ctrToUse);
	}
}

#endif
