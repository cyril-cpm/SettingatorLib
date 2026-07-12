#include "Definitions.h"

#if CONFIG_STR_HAS_BRIDGE
#include "Slave.h"
#include "Communicator.h"
#include "ESPNowCommunicator.h"
#include "Master.h"

#include "sdkconfig.h"
#include <variant>

static const char* tag = "SLAVE";

uint16_t Slave::GetLinkInfoSize() const
{
	const auto& ctrToUse = fCTRArray[fCTRToUse];

	if (ctrToUse)
	{
		return std::visit([](auto&& ctr) -> uint16_t {
				return ctr.get().GetLinkInfoSize() + 8;
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

		memcpy(&messageBuffer[index + 2], fEMac.data(), 6);

		std::visit([index](auto&& ctr) {

				ctr.get().WriteLinkInfoToBuffer(index + 8);
			
			}, ctrToUse);
	}
}

// void Slave::HandleSlaveIDRequest()
// {
	
// }
#endif
