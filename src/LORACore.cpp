#include "Definitions.h"

#if STR_HAS_LORA
#include "LORACore.h"
#include "LinkUtils.h"

void LORACore::_Run()
{
	while (true)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		_ReadUart();
		if (_FetchMessage())
			TreatLinkMessage<LORACore>(
					*this,
					&fMidBuf[5],
					(fMidBuf[1] << 8) + fMidBuf[2],
					{fMidBuf[3], fMidBuf[4]},
					true,
					0,
					0,
					0
				);
	}
}

#endif
