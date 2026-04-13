#pragma once

#include "Definitions.h"
#include <cstdint>

#if STR_HAS_UART

#include "Communicator.h"
#include "UARTCore.h"
#include "esp_err.h"
#include <driver/uart.h>
#include <initializer_list>


class Message;

class UARTCTR: public ICTR
{
	public:

		UARTCTR(UARTCore& core) : fCore(core) {}

		void	UpdateImpl();

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
#endif
