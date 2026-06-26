#pragma once

#include "Definitions.h"
#include "esp_err.h"
#include "sdkconfig.h"
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

#if STR_UART0
		static UARTCore& GetUART0Instance()
		{
			static UARTCore instance(UART_NUM_0,
					CONFIG_STR_UART0_TX_PIN,
					CONFIG_STR_UART0_RX_PIN,
					CONFIG_STR_UART0_BAUDRATE);

			return instance;
		}
#endif

#if STR_UART1
		static UARTCore& GetUART1Instance()
		{
			static UARTCore instance(UART_NUM_1,
					CONFIG_STR_UART1_TX_PIN,
					CONFIG_STR_UART1_RX_PIN,
					CONFIG_STR_UART1_BAUDRATE);

			return instance;
		}
#endif

#if STR_UART2
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
			size_t size;

			ESP_ERROR_CHECK(uart_get_buffered_data_len(fUartPort, &size));

			if (!size)
				return;

			if (size > fBuf.GetRemainingLength())
				size = fBuf.GetRemainingLength();
			
			uint16_t contingousRemaining = fBuf.GetContingousRemainingLength();

			if (size > contingousRemaining)
			{
				uint16_t overflow = size - contingousRemaining;

				if (overflow >= fBuf.GetHeadPos())
				{
					ESP_LOGI("UARTCore", "UARTCore::Read()");
					ESP_LOGI("UARTCore", "Not enough space left in CircularBuffer (%d)",
							fBuf.GetRemainingLength());

					fBuf.LogWholeBuffer();
					ESP_LOGI("UARTCore", "fHead: %t\tfTail: %d",
							fBuf.GetHeadPos(),
							fBuf.GetTailPos());
					return;
				}

				int read = uart_read_bytes(fUartPort,
												fBuf.GetTailPtr(),
												contingousRemaining,
												0);
				if (read >= 0)
				{
					fBuf.OffsetTail(read);

					read = uart_read_bytes(fUartPort, fBuf.GetTailPtr(), overflow, 0);
					
					if (read >= 0)
						fBuf.OffsetTail(read);
				}
			}
			else
			{
				int read = uart_read_bytes(fUartPort, fBuf.GetTailPtr(), size, 0);
				
				if (read >=0)
					fBuf.OffsetTail(read);
			}

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
