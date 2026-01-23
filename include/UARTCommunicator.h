#pragma once

#if defined(ESP_PLATFORM)

#include "Communicator.h"
#include <driver/uart.h>

class Message;

class UARTCTR: public ICTR
{
    public:

    static UARTCTR CreateInstance(int baudrate = 115200);

    int     WriteImpl(Message& buf);
    void    UpdateImpl();

    private:

    UARTCTR(int baudrate);
	UARTCTR() = default;

    void            _removeBufferBeginBytes(size_t numberBytes);

    uint8_t*        fUartBuffer = nullptr;
    size_t          fUartBufferSize = 0;
    uart_port_t     fUartPort = UART_NUM_0;

    int             fRxBufferSize = 1024;
    int             fTxBufferSize = 1024;
};

#endif
