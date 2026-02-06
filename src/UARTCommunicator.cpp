#if defined(ESP_PLATFORM)

#include "UARTCommunicator.h"
#include <stdlib.h>
#include <cstring>
#include <esp_log.h>
#include "Message.h"
#include "MiscDef.h"

static const char* tag("UARTCTR");

UARTCTR UARTCTR::CreateInstance(int baudrate)
{
	return UARTCTR(baudrate);
}

UARTCTR::UARTCTR(int baudrate)
{
	uart_config_t uartConfig = {
		.baud_rate = baudrate,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 0,
	};

	auto err = uart_set_pin(fUartPort, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
		UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	// if (err != ESP_OK)

	ESP_ERROR_CHECK(uart_param_config(fUartPort, &uartConfig));

	ESP_ERROR_CHECK(uart_driver_install(fUartPort, fRxBufferSize, fTxBufferSize,
										0, nullptr, 0));
}

int UARTCTR::WriteImpl(Message& buf)
{
	esp_err_t err = uart_write_bytes(fUartPort, buf.GetBufPtr(), buf.GetLength());

	if (err == ESP_FAIL)
	{
	}

	return 0;
}

void UARTCTR::UpdateImpl()
{
	size_t bufferDataLen = 0;
	
	ESP_ERROR_CHECK(uart_get_buffered_data_len(fUartPort, &bufferDataLen));

	if (bufferDataLen)
	{
		LOG("data availlable %d", bufferDataLen);

		LOG("buff size: %d", fUartBuffer.size());
		LOG("buff capacity: %d", fUartBuffer.capacity());
		
		uint16_t prevSize = fUartBuffer.size();
		fUartBuffer.resize(fUartBuffer.size() + bufferDataLen);

		LOG("buff size: %d", fUartBuffer.size());
		LOG("buff capacity: %d", fUartBuffer.capacity());

		uart_read_bytes(fUartPort, fUartBuffer.data() + prevSize, bufferDataLen, 0);

		LOG_BUFFER_HEX(fUartBuffer.data(), fUartBuffer.size());

		int i = 0;
		for (i = 0; i < fUartBuffer.size() && fUartBuffer[i] != Message::Frame::Start; i++);

		LOG("i = %d", i);

		if (i != 0)
			fUartBuffer.erase(fUartBuffer.begin(), fUartBuffer.begin() + i);

		LOG("buff size: %d", fUartBuffer.size());
		LOG("buff capacity: %d", fUartBuffer.capacity());

		if (fUartBuffer.size() >= 5)
		{
			uint16_t msgSize = (fUartBuffer[1] << 8) + fUartBuffer[2];
			LOG("msgSize %d", msgSize);

			if (fUartBuffer.size() >= msgSize)
			{
				LOG("message _reveice");
				_receive(Message(fUartBuffer.data(), msgSize));
				fUartBuffer.erase(fUartBuffer.begin(), fUartBuffer.begin() + msgSize);
			}
		}

	}
}

#endif
