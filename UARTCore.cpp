#include "Definitions.h"

#if STR_HAS_UART
#include "UARTCore.h"
#include "MiscDef.h"
#include "Buffer.h"
#include "driver/uart.h"
#include "hal/uart_types.h"
#include <initializer_list>

static const char* tag = "UARTCore";

UARTCore::UARTCore(uart_port_t port, int tx, int rx, int baudrate)
	:
		fUartPort(port),
		fRx(rx),
		fTx(tx),
		fBaudrate(baudrate)
{}

void UARTCore::Init() 
{
	LOG("InitImpl");

	uart_config_t uartConfig = {
		.baud_rate = fBaudrate,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 0,
	};

	ESP_ERROR_CHECK(uart_set_pin(fUartPort, fTx, fRx,
		UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

	ESP_ERROR_CHECK(uart_param_config(fUartPort, &uartConfig));

	ESP_ERROR_CHECK(uart_driver_install(fUartPort,
				CONFIG_STR_CIRCULAR_BUFFER_SIZE,
				CONFIG_STR_MESSAGE_BUFFER_SIZE,
				0, nullptr, 0));
}
#endif
