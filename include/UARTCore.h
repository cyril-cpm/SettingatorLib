#pragma once

#include "Definitions.h"

#if STR_HAS_UART
#include "Core.h"
#include "Buffer.h"
#include "UARTUtils.h"

#include <driver/uart.h>
#include <initializer_list>
#include "esp_err.h"
#include "sdkconfig.h"
#include <cstdint>


class UARTCore : public ICore
{
	public:

#if STR_HAS_UART0
		static UARTCore& GetUART0Instance()
		{
			static UARTCore instance(UART_NUM_0,
					CONFIG_STR_UART0_TX_PIN,
					CONFIG_STR_UART0_RX_PIN,
					CONFIG_STR_UART0_BAUDRATE);

			return instance;
		}
#endif

#if STR_HAS_UART1
		static UARTCore& GetUART1Instance()
		{
			static UARTCore instance(UART_NUM_1,
					CONFIG_STR_UART1_TX_PIN,
					CONFIG_STR_UART1_RX_PIN,
					CONFIG_STR_UART1_BAUDRATE);

			return instance;
		}
#endif

#if STR_HAS_UART2
		static UARTCore& GetUART2Instance()
		{
			static UARTCore instance(UART_NUM_2,
					CONFIG_STR_UART2_TX_PIN,
					CONFIG_STR_UART2_RX_PIN,
					CONFIG_STR_UART2_BAUDRATE);

			return instance;
		}
#endif

		int Write(std::initializer_list<uint8_t> message) const {
			return uart_write_bytes(fUartPort, message.begin(), message.size());
		}

		int Write() const {
			return uart_write_bytes(fUartPort, messageBuffer.data(), messageBuffer.len());
		}

		void Read() {
			ReadUart(fUartPort, fBuf);
		}

		void	Init();

	private:

		UARTCore(uart_port_t port, int tx, int rx, int baudrate)
			: 
				fUartPort(port),
				fRx(tx),
				fTx(rx),
				fBaudrate(baudrate)
		{}

		uart_port_t		fUartPort;
		int				fRx;
		int				fTx;
		int				fBaudrate;
};
#endif
