#pragma once

#include "Definitions.h"

#if CONFIG_STR_HAS_BRIDGE

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <variant>
#include <stdatomic.h>

#include "Buffer.h"
#include "Communicator.h"
#include "Message.h"

#if CONFIG_STR_SLAVE_ESPNOW
#include "ESPNowCommunicator.h"
#endif

#if STR_SLAVE_HAS_UART
#include "UARTCommunicator.h"
#endif


enum SlaveCTREnum {

#if CONFIG_STR_SLAVE_ESPNOW
	SLAVE_CTR_ESPNOW,
#endif

#if CONFIG_STR_SLAVE_UART0
	SLAVE_CTR_UART0,
#endif

#if CONFIG_STR_SLAVE_UART1
	SLAVE_CTR_UART1,
#endif

#if CONFIG_STR_SLAVE_UART2
	SLAVE_CTR_UART2,
#endif

	SLAVE_CTR_MAX
};

using SlaveCTR = CTRVariant<
	STRIP_FIRST_COMMA(
			dummy

#if CONFIG_STR_SLAVE_ESPNOW
			,std::reference_wrapper<ESPNowCTR>
#endif

#if STR_SLAVE_HAS_UART
			,std::reference_wrapper<UARTCTR>
#endif
		)
	>;

class Slave
{
    public:

	Slave() : fCTRArray({
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
					)

			})	{}

    uint8_t 	GetID();
    
	bool    	HasSubSlave(uint8_t id) const {
#if CONFIG_STR_NB_SUBSLAVE
		uint8_t i = 0;
		for (uint8_t subSlaveID : fSubSlave)
		{
			if (i >= fSubSlaveCount)
				break;
			i++;
			if (id == subSlaveID)
				return true;
		}
#endif
		return false;
	}

#if CONFIG_STR_NB_SUBSLAVE
    void    	AddSubSlave(uint8_t id);
#endif
    void    	SetID(uint8_t id);
	uint16_t	GetLinkInfoSize() const;
	void		WriteLinkInfoToBuffer(uint16_t msgBuffer) const;

	void		SetEMac(const std::array<uint8_t, 6>&& eMac) {
		fEMac = std::move(eMac);
	}

	const std::array<uint8_t, 6>& GetEMac() const {
		return fEMac;
	}

	bool		HasCTR(SlaveCTREnum type) {
		return (bool)fCTRArray[type];
	}

#if CONFIG_STR_SLAVE_ESPNOW
	ESPNowCTR&	GetESPNowCTR() {
		return std::get<SLAVE_CTR_ESPNOW>(fCTRArray[SLAVE_CTR_ESPNOW]);
	}
#endif

	void		SetWaitingForID(const bool value = true) {
		fWaitingForID = value;
	}

	bool		IsWaitingForID() const {
		return fWaitingForID;
	}

	void		Write() const {
		const auto& ctrToUse = fCTRArray[fCTRToUse];

		if (ctrToUse)
			ctrToUse.Write();
	}


	void		Write(std::initializer_list<uint8_t> message) const {
		const auto& ctrToUse = fCTRArray[fCTRToUse];

		if (ctrToUse)
			ctrToUse.Write(message);
	}
	
	void		PlanifySendInitRequest() {
		atomic_store(&fShouldSendInitRequest, true);
	}

	void		HandleSendInitRequest() {
		if (atomic_exchange(&fShouldSendInitRequest, false))
			SendInitRequest();
	}

	void		SendInitRequest() {
		messageBuffer[0] = Message::Frame::Start;
		messageBuffer[1] = 0;
		messageBuffer[2] = 0x08;
		messageBuffer[3] = 0;
		messageBuffer[4] = fSlaveID;
		messageBuffer[5] = Message::Type::InitRequest;
		messageBuffer[6] = 0;
		messageBuffer[7] = Message::Frame::End;

		messageBuffer.SetLen(0x08);

		Write();
	}

	void		Activate() { fActivated = true; }

	constexpr explicit operator bool() const noexcept { return fActivated; }

    private:
    uint8_t     			fSlaveID = 0;
	
	std::array<SlaveCTR, SlaveCTREnum::SLAVE_CTR_MAX> 	fCTRArray;
	uint8_t		fCTRToUse = 0;

	bool					fWaitingForID = false;
	std::array<uint8_t, 6>	fEMac;

	bool					fActivated = false;

	std::atomic_bool					fShouldSendInitRequest = false;

#if CONFIG_STR_NB_SUBSLAVE
    std::array<uint8_t, CONFIG_STR_NB_SUBSLAVE> fSubSlave;
	uint8_t										fSubSlaveCount = 0;
#endif
};

using OptSlaveRef = std::optional<std::reference_wrapper<Slave>>;

extern bool initEspNowBroadcasted;

#endif
