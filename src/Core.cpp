#include "Core.h"

#if STR_HAS_ESPNOW
#include "ESPNowCore.h"
#endif

#if STR_HAS_UART
#include "UARTCore.h"
#endif

#if STR_HAS_LORA
#include "LORACore.h"
#endif

#include <functional>

std::array<std::reference_wrapper<ICore>, CORE_MAX> coreArray {

	STRIP_FIRST_COMMA(
			dummy

#if STR_HAS_ESPNOW
			,ESPNowCore::GetInstance()
#endif

#if STR_HAS_UART0
			,UARTCore::GetUART0Instance()
#endif

#if STR_HAS_UART1
			,UARTCore::GetUART1Instance()
#endif

#if STR_HAS_UART2
			,UARTCore::GetUART2Instance()
#endif

#if STR_HAS_LORA
			,LORACore::GetInstance()
#endif
		)

};

void		InitCores()
{
	if (!mainTaskHandle)
		mainTaskHandle = xTaskGetCurrentTaskHandle();

#if STR_HAS_ESPNOW
	ESPNowCore::GetInstance().Init();
#endif

#if STR_HAS_UART0
	UARTCore::GetUART0Instance().Init();
#endif

#if STR_HAS_UART1
	UARTCore::GetUART1Instance().Init();
#endif

#if STR_HAS_UART2
	UARTCore::GetUART2Instance().Init();
#endif

#if STR_HAS_LORA
	LORACore::GetInstance().Init();
#endif
}

