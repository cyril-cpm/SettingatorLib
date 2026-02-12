#pragma once

#include "Definitions.h"
#include <driver/uart.h>
#include <initializer_list>
#include "Buffer.h"
#include "hal/uart_types.h"
#include "Core.h"

class UARTCore : public ICore
{
	public:

		static UARTCore& GetDefaultUARTInstance()
		{
			static UARTCore	instance(UART_NUM_0,
				UART_PIN_NO_CHANGE,
				UART_PIN_NO_CHANGE,
				115200);

			return instance;
		}

		UARTCore(uart_port_t port, int tx, int rx, int baudrate);

		void Write(std::initializer_list<uint8_t> message) const {
			uart_write_bytes(fUartPort, message.begin(), message.size());
		}

		void Write() const {
			uart_write_bytes(fUartPort, messageBuffer.data(), messageBuffer.len());
		}

	private:

		uart_port_t		fUartPort;
};
