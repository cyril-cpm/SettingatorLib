#pragma once

#include "Communicator.h"
#include "Definitions.h"
#include "UARTCore.h"
#include "esp_err.h"
#include <driver/uart.h>
#include <initializer_list>


class Message;

class UARTCTR: public ICTR
{
	public:

		using ICTR::ICTR;

		void	UpdateImpl();

	private:

		UARTCTR() = delete;
};
