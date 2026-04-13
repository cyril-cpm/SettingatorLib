#include "UARTCommunicator.h"

#if STR_HAS_UART

#include <stdlib.h>
#include <cstring>
#include <esp_log.h>
#include "Message.h"
#include "MiscDef.h"

static const char* tag("UARTCTR");

void UARTCTR::UpdateImpl()
{
	// size_t bufferDataLen = 0;
	//
	// ESP_ERROR_CHECK(uart_get_buffered_data_len(fUartPort, &bufferDataLen));
	//
	// if (bufferDataLen)
	// {
	// 	LOG("data availlable %d", bufferDataLen);
	//
	// 	LOG("buff size: %d", fUartBuffer.size());
	// 	LOG("buff capacity: %d", fUartBuffer.capacity());
	//
	// 	uint16_t prevSize = fUartBuffer.size();
	// 	fUartBuffer.resize(fUartBuffer.size() + bufferDataLen);
	//
	// 	LOG("buff size: %d", fUartBuffer.size());
	// 	LOG("buff capacity: %d", fUartBuffer.capacity());
	//
	// 	uart_read_bytes(fUartPort, fUartBuffer.data() + prevSize, bufferDataLen, 0);
	//
	// 	LOG_BUFFER_HEX(fUartBuffer.data(), fUartBuffer.size());
	//
	// 	int i = 0;
	// 	for (i = 0; i < fUartBuffer.size() && fUartBuffer[i] != Message::Frame::Start; i++);
	//
	// 	LOG("i = %d", i);
	//
	// 	if (i != 0)
	// 		fUartBuffer.erase(fUartBuffer.begin(), fUartBuffer.begin() + i);
	//
	// 	LOG("buff size: %d", fUartBuffer.size());
	// 	LOG("buff capacity: %d", fUartBuffer.capacity());
	//
	// 	if (fUartBuffer.size() >= 5)
	// 	{
	// 		uint16_t msgSize = (fUartBuffer[1] << 8) + fUartBuffer[2];
	// 		LOG("msgSize %d", msgSize);
	//
	// 		if (fUartBuffer.size() >= msgSize)
	// 		{
	// 			LOG("message _reveice");
	// 			_receive(Message(fUartBuffer.data(), msgSize));
	// 			fUartBuffer.erase(fUartBuffer.begin(), fUartBuffer.begin() + msgSize);
	// 		}
	// 	}
	//
	// }
}

#endif
