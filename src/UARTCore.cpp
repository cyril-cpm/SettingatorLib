#include "UARTCore.h"
#include "Buffer.h"
#include "driver/uart.h"
#include "hal/uart_types.h"
#include <initializer_list>

UARTCore::UARTCore(uart_port_t port, int tx, int rx, int baudrate)
	:
		ICore(),
		fUartPort(port)
{
	uart_config_t uartConfig = {
		.baud_rate = baudrate,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 0,
	};

	ESP_ERROR_CHECK(uart_set_pin(fUartPort, tx,rx,
		UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

	ESP_ERROR_CHECK(uart_param_config(fUartPort, &uartConfig));

	ESP_ERROR_CHECK(uart_driver_install(fUartPort,
				UART_RX_BUFFER_SIZE,
				UART_TX_BUFFER_SIZE,
				0, nullptr, 0));
}
