#pragma once

#include "Definitions.h"

#if STR_HAS_UART

#include "Communicator.h"
#include "UARTCore.h"
#include "esp_err.h"
#include <driver/uart.h>
#include <initializer_list>

enum UartCTREnum {

#if CONFIG_STR_MASTER_UART0
	UART_CTR_UART0,
#endif

#if CONFIG_STR_MASTER_UART1
	UART_CTR_UART1,
#endif

#if CONFIG_STR_MASTER_UART2
	UART_CTR_UART2,
#endif

	UART_CTR_MAX
};

class Message;

class UARTCTR: public ICTR
{
	public:

		UARTCTR(UARTCore& core) : fCore(core) {
			fActivated = true;
		}

		void	Update() {

		}

		int		Write(std::initializer_list<uint8_t> message) const {
			return fCore.Write(message);
		}

		int		Write() const {
			return fCore.Write();
		}

		uint16_t	GetLinkInfoSizeImpl() const { return 0; }
		void	WriteLinkInfoToBufferImpl(uint16_t index) const {}

	private:

		UARTCTR() = delete;

		UARTCore&	fCore;
};

extern std::array<UARTCTR, NB_UART_CTR> uartCtrArray;

#endif
