#pragma once

#include "Definitions.h"
#include "esp_err.h"
#include <cstdint>

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

		void Read() {
			size_t size;

			ESP_ERROR_CHECK(uart_get_buffered_data_len(fUartPort, &size));

			if (!size)
				return;

			ESP_LOGI("UART", "data received");

			uint16_t remaining = fBuf.GetRemainingLength();
			uint16_t contingousRemaining = fBuf.GetContingousRemainingLength();

			ESP_LOGI("UART", "%d size", size);
			ESP_LOGI("UART", "%d remaining", remaining);
			ESP_LOGI("UART", "%d contingous", contingousRemaining);

			if (size > remaining)
				size = remaining;

			if (size <= contingousRemaining)
			{
				int read = uart_read_bytes(fUartPort,
											fBuf.GetTail(),
											size,
											0);
				ESP_LOGI("UARTCore", "%d bits read", read);
				if (read >= 0)
				{
					fBuf.OffsetTail(read);
					fBuf.LogContent();
				}
			}
			else
			{
				ESP_ERROR_CHECK(uart_read_bytes(fUartPort,
												fBuf.GetAfterTail(),
												contingousRemaining,
												0));
				fBuf.OffsetTail(contingousRemaining);
				ESP_ERROR_CHECK(uart_read_bytes(fUartPort,
												fBuf.GetAfterTail(),
												size - contingousRemaining,
												0));
				fBuf.OffsetTail(size - contingousRemaining);
			}
		}

		void	Init();

	private:

		uart_port_t		fUartPort;
		int				fRx;
		int				fTx;
		int				fBaudrate;
};
#endif
