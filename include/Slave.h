#pragma once

#include "Definitions.h"

#if CONFIG_STR_HAS_BRIDGE

#include <atomic>
#include <array>
#include <functional>
#include <variant>
#include <stdatomic.h>

#include "Buffer.h"
#include "Communicator.h"
#include "CommunicatorHandler.h"
#include "Message.h"
#include "Master.h"

#if CONFIG_STR_SLAVE_ESPNOW
#include "ESPNowCommunicator.h"
#endif

#if STR_SLAVE_HAS_UART
#include "UARTCommunicator.h"
#endif

#if CONFIG_STR_SLAVE_LORA
#include "LORACommunicator.h"
#endif

using SlaveCTR = CTRVariant<
	STRIP_FIRST_COMMA(
			dummy

#if CONFIG_STR_SLAVE_ESPNOW
			,std::reference_wrapper<ESPNowCTR>
#endif

#if STR_SLAVE_HAS_UART
			,std::reference_wrapper<UARTCTR>
#endif

#if CONFIG_STR_SLAVE_LORA
			,std::reference_wrapper<LORACTR>
#endif
		)
	>;

class Slave : public CTRHandler<SlaveCTR, SlaveCTREnum::SLAVE_CTR_MAX>
{
    public:

	Slave()
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

			})	{}

    void    	SetID(uint8_t id) { fSlaveID = id; }
    uint8_t 	GetID() const { return fSlaveID; }

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
    void    	AddSubSlave(uint8_t id) {
		if (fSubSlaveCount <= CONFIG_STR_NB_SUBSLAVE)
		{
			fSubSlave[fSubSlaveCount] = id;
			fSubSlaveCount++;
		}
	}
#endif
	uint16_t	GetLinkInfoSize() const;
	void		WriteLinkInfoToBuffer(uint16_t msgBuffer) const;

	void		SetEMac(const std::array<uint8_t, 6>&& eMac) {
		fEMac = std::move(eMac);
	}

	const std::array<uint8_t, 6>& GetEMac() const {
		return fEMac;
	}

	void		SetWaitingForID(const bool value = true) {
		fWaitingForID = value;
	}

	bool		IsWaitingForID() const {
		return fWaitingForID;
	}

	void		PlanifySendInitRequest() {
		atomic_store(&fShouldSendInitRequest, true);
	}

	void		HandleSendInitRequest() {
		if (atomic_exchange(&fShouldSendInitRequest, false))
			SendInitRequest();
	}

	void		PlanifySlaveIDRequest() {
		atomic_store(&fShouldReqestSlaveID, true);
	}

	void		HandleSlaveIDRequest() {
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

	void Update() {
		for (auto& ctr : fCTRArray)
			ctr.Update();

		HandleSlaveIDRequest();
		HandleSendInitRequest();
	}

	void		SendInitRequest() {
		Write({
			Message::Frame::Start,
			0,
			0x08,
			0,
			fSlaveID,
			Message::Type::InitRequest,
			0,
			Message::Frame::End
		});
	}

	void		Activate() { fActivated = true; }

	constexpr explicit operator bool() const noexcept { return fActivated; }

    private:
    uint8_t     			fSlaveID = 0;

	bool					fWaitingForID = false;
	std::array<uint8_t, 6>	fEMac;

	bool					fActivated = false;

	std::atomic_bool		fShouldSendInitRequest = false;
	std::atomic_bool		fShouldReqestSlaveID = false;

#if CONFIG_STR_NB_SUBSLAVE
    std::array<uint8_t, CONFIG_STR_NB_SUBSLAVE> fSubSlave;
	uint8_t										fSubSlaveCount = 0;
#endif
};

using OptSlaveRef = std::optional<std::reference_wrapper<Slave>>;

inline std::array<Slave, CONFIG_STR_NB_SLAVE> slaveArray;
inline uint8_t nbInitialisedSlave = 0;

#endif
