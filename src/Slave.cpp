#include "Definitions.h"

#if CONFIG_STR_HAS_BRIDGE
#include "Slave.h"
#include "Communicator.h"
#include "ESPNowCommunicator.h"
#include "Master.h"

#include "sdkconfig.h"
#include <variant>

bool initEspNowBroadcasted = false;

Slave::Slave()
	:
		CTRHandler<SlaveCTR, SlaveCTREnum::SLAVE_CTR_MAX>({
				STRIP_FIRST_COMMA(
						dummy

#if CONFIG_STR_SLAVE_ESPNOW
						,espNowCtrArray[registeredEspNowCtr++]
#endif

#if CONFIG_STR_SLAVE_UART0
						,uartCtrArray[UartCTREnum::UART_CTR_UART0]
#endif

#if CONFIG_STR_SLAVE_UART1
						,uartCtrArray[UartCTREnum::UART_CTR_UART1]
#endif

#if CONFIG_STR_SLAVE_UART2
						,uartCtrArray[UartCTREnum::UART_CTR_UART2]
#endif

#if CONFIG_STR_SLAVE_LORA
						,loraCtrArray[registeredLoRaCtr++]
#endif
					)

			})	
{}


uint16_t Slave::GetLinkInfoSize() const
{
	const auto& ctrToUse = fCTRArray[fCTRToUse];

	if (ctrToUse)
	{
		return std::visit([](auto&& ctr) -> uint16_t {
				return ctr.get().GetLinkInfoSize() + 2;
			}, ctrToUse);
	}

	return 0;
}

void Slave::WriteLinkInfoToBuffer(uint16_t index) const
{
	const auto& ctrToUse = fCTRArray[fCTRToUse];

	if (ctrToUse)
	{
		messageBuffer[index] = GetLinkInfoSize();
		messageBuffer[index + 1] = fSlaveID;

		std::visit([index](auto&& ctr) {

				ctr.get().WriteLinkInfoToBuffer(index + 2);
			
			}, ctrToUse);
	}
}

void Slave::HandleSendInitRequest()
{
	if (atomic_exchange(&fShouldReqestSlaveID, false))
	{
		Master::GetInstance().Write({
							Message::Frame::Start,
							0x00,
							0x07,
							0,
							0,
							Message::Type::SlaveIDRequest,
							Message::Frame::End
						});
		fWaitingForID = true;
	}
}

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
