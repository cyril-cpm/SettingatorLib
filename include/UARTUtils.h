#pragma once

#include "Definitions.h"

#if STR_HAS_UART || STR_HAS_LORA

#include "hal/uart_types.h"

void		ReadUart(uart_port_t uartPort, CircularBuffer& buf);

#endif

