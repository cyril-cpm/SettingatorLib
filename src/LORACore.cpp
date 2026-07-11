#include "Definitions.h"

#if STR_HAS_LORA
#include "LORACore.h"
#include "LinkUtils.h"

#include <queue>

void LORACore::_Run()
{
	uart_event_t event;
	while (true)
	{
		// ESP_LOGD("LORACore", "Waiting Notification");
		// ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if(xQueueReceive(fUartQueue, (void*)&event, (TickType_t)portMAX_DELAY))
		{
			_ReadUart();
			if (_FetchMessage())
			{
				TreatLinkMessage<LORACore>(
						*this,
						&fMidBuf[5],
						(fMidBuf[1] << 8) + fMidBuf[2] - 6,
						{fMidBuf[3], fMidBuf[4]},
						(int8_t)fMidBuf[(fMidBuf[1] << 8) + fMidBuf[2]],
						0,
						0
					);
				fMidBuf.OffsetHead((fMidBuf[1] << 8) + fMidBuf[2] + 1);
			}
			//flush le circularBuffer si fetch false et buffer plein
		}
	}
}

#endif
