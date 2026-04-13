#pragma once

#include "Definitions.h"

#if STR_HAS_UART
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
#
#if CONFIG_STR_UART0
		static UARTCore& GetUART0Instance()
		{
			static UARTCore instance(UART_NUM_0,
					CONFIG_STR_UART0_TX_PIN,
					CONFIG_STR_UART0_RX_PIN,
					CONFIG_STR_UART0_BAUDRATE);

			return instance;
		}
#endif


#if CONFIG_STR_UART1
		static UARTCore& GetUART1Instance()
		{
			static UARTCore instance(UART_NUM_0,
					CONFIG_STR_UART1_TX_PIN,
					CONFIG_STR_UART1_RX_PIN,
					CONFIG_STR_UART1_BAUDRATE);

			return instance;
		}
#endif

#if CONFIG_STR_UART2
		static UARTCore& GetUART2Instance()
		{
			static UARTCore instance(UART_NUM_0,
					CONFIG_STR_UART2_TX_PIN,
					CONFIG_STR_UART2_RX_PIN,
					CONFIG_STR_UART2_BAUDRATE);

			return instance;
		}
#endif

		UARTCore(uart_port_t port, int tx, int rx, int baudrate);

		int Write(std::initializer_list<uint8_t> message) const {
			return uart_write_bytes(fUartPort, message.begin(), message.size());
		}

		int Write() const {
			return uart_write_bytes(fUartPort, messageBuffer.data(), messageBuffer.len());
		}

		void	Init();

	private:

		uart_port_t		fUartPort;
		int				fRx;
		int				fTx;
		int				fBaudrate;
};
#endif
