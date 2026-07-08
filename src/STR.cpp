#include "STR.h"
#include "Slave.h"
#include "esp_log_buffer.h"
#include "freertos/idf_additions.h"

#if STR_HAS_ESPNOW
#include "ESPNowCore.h"
#endif

#if STR_HAS_UART
#include "UARTCore.h"
#endif


static const char* tag = "STR";

#if STR_HAS_UART || STR_HAS_LORA
void	ReadUart(uart_port_t uartPort, CircularBuffer& buf)
{
	ESP_LOGD("ReadUart", "Bah alors ?");
	size_t size;

	ESP_ERROR_CHECK(uart_get_buffered_data_len(uartPort, &size));

	if (!size)
		return;

	ESP_LOGD("ReadUart", "there is uart to read");

	if (size > buf.GetRemainingLength())
		size = buf.GetRemainingLength();
		
	uint16_t contingousRemaining = buf.GetContingousRemainingLength();

	if (size > contingousRemaining)
	{
		uint16_t overflow = size - contingousRemaining;

		if (overflow >= buf.GetHeadPos())
		{
			ESP_LOGI("UARTCore", "UARTCore::Read()");
			ESP_LOGI(
					"UARTCore", "Not enough space left in CircularBuffer (%d)",
					buf.GetRemainingLength()
				);

			buf.LogWholeBuffer();
			ESP_LOGI(
					"UARTCore", "fHead: %t\tfTail: %d",
					buf.GetHeadPos(),
					buf.GetTailPos()
				);
			return;
		}

		int read = uart_read_bytes(
				uartPort,
				buf.GetTailPtr(),
				contingousRemaining,
				0
			);

		if (read >= 0)
		{
			buf.OffsetTail(read);

			read = uart_read_bytes(uartPort, buf.GetTailPtr(), overflow, 0);
			
			if (read >= 0)
				buf.OffsetTail(read);
		}
	}
	else
	{
		int read = uart_read_bytes(uartPort, buf.GetTailPtr(), size, 0);
		
		if (read >=0)
			buf.OffsetTail(read);
	}
}
#endif
