#include "UARTCommunicator.h"
#include "Definitions.h"

#if STR_HAS_UART

#include <stdlib.h>
#include <cstring>
#include <esp_log.h>
#include "Message.h"
#include "MiscDef.h"

static const char* tag("UARTCTR");

std::array<UARTCTR, NB_UART_CTR> uartCtrArray({
		STRIP_FIRST_COMMA(
			dummy

#if CONFIG_STR_MASTER_UART0 || CONFIG_STR_SLAVE_UART0
			,UARTCore::GetUART0Instance()
#endif

#if CONFIG_STR_MASTER_UART1 || CONFIG_STR_SLAVE_UART1
			,UARTCore::GetUART1Instance()
#endif

#if CONFIG_STR_MASTER_UART2 || CONFIG_STR_SLAVE_UART2
			,UARTCore::GetUART2Instance()
#endif
		)
	});

#endif
