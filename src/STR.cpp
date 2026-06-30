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

std::array<std::reference_wrapper<ICore>, CORE_MAX> coreArray {

	STRIP_FIRST_COMMA(
			dummy

#if STR_HAS_ESPNOW
			,ESPNowCore::GetInstance()
#endif

#if STR_UART0
			,UARTCore::GetUART0Instance()
#endif

#if STR_UART1
			,UARTCore::GetUART1Instance()
#endif

#if STR_ART2
			,UARTCore::GetUART2Instance()
#endif

#if STR_HAS_LORA
			,LORACore::GetInstance()
#endif
		)

};

#if CONFIG_STR_HAS_BRIDGE
std::array<Slave, CONFIG_STR_NB_SLAVE> slaveArray;
uint8_t nbInitialisedSlave = 0;

OptSlaveRef GetSlaveForID(const uint8_t id)
{
	for (auto& slave : slaveArray)
	{
		if (!slave)
			break;

		if (slave.GetID() == id || slave.HasSubSlave(id))
			return slave;
	}

	return std::nullopt;
}

OptSlaveRef GetSlaveForEMac(const std::array<uint8_t, 6> &eMac)
{
	LOG("Looking for slave with EMac");
	ESP_LOG_BUFFER_HEX(tag, eMac.data(), eMac.size());

	for (auto& slave : slaveArray)
	{
		if (!slave)
			break;

		if (eMac == slave.GetEMac())
			return slave;
	}

	return std::nullopt;
}

OptSlaveRef CreateSlave(std::array<uint8_t, 6> &&eMac, uint8_t id)
{
	ESP_LOGI("STR", "CreateSlave");
	if (nbInitialisedSlave < CONFIG_STR_NB_SLAVE)
	{
		ESP_LOGI("STR", "There is room dfor a slave");
		Slave& slave = slaveArray[nbInitialisedSlave];
		slave.SetEMac(std::move(eMac));
		slave.SetID(id);
		slave.Activate();
		nbInitialisedSlave++;

		if (!id)
			slave.PlanifySlaveIDRequest();

		return slave;
	}
	else
		ESP_LOGI("STR", "To much slave initialised: %d", nbInitialisedSlave);
	return std::nullopt;
}
#endif

void		InitCores()
{
	if (!mainTaskHandle)
		mainTaskHandle = xTaskGetCurrentTaskHandle();

#if STR_HAS_ESPNOW
	ESPNowCore::GetInstance().Init();
#endif

#if STR_HAS_UART0
	UARTCore::GetUART0Instance().Init();
#endif

#if STR_HAS_UART1
	UARTCore::GetUART1Instance().Init();
#endif

#if STR_HAS_UART2
	UARTCore::GetUART2Instance().Init();
#endif

#if STR_HAS_LORA
	LORACore::GetInstance().Init();
#endif
}

#if STR_HAS_UART || STR_HAS_LORA
void	ReadUart(uart_port_t uartPort, CircularBuffer& buf)
{
	size_t size;

	ESP_ERROR_CHECK(uart_get_buffered_data_len(uartPort, &size));

	if (!size)
		return;

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
