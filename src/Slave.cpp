#include "Slave.h"
#include "Communicator.h"
#include "ESPNowCommunicator.h"
#include "STR.h"
#include "sdkconfig.h"
#include <cstddef>
#include <variant>

bool initEspNowBroadcasted = false;

ICTR_t* Slave::GetCTR()
{
    return nullptr;//&fCTR;
}

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
	const auto& ctrToUse = fCTR[fCTRToUse];

	if (ctrToUse)
	{
		return std::visit([](auto&& ctr) -> uint16_t {

				using T = std::decay_t<decltype(ctr)>;

				if constexpr (!std::is_same_v<T, std::monostate>)
					return ctr.GetLinkInfoSize() + 2;

				else
					return 0;
			}, *ctrToUse);
	}

	return 0;
}

void Slave::WriteLinkInfoToBuffer(uint16_t index) const
{
	const auto& ctrToUse = fCTR[fCTRToUse];

	if (ctrToUse)
	{
		messageBuffer[index] = GetLinkInfoSize();
		messageBuffer[index + 1] = fSlaveID;

		std::visit([index](auto&& ctr) {

				using T = std::decay_t<decltype(ctr)>;

				if constexpr (!std::is_same_v<T, std::monostate>)
					ctr.WriteLinkInfoToBuffer(index + 2);
			
			}, *ctrToUse);
	}
}
