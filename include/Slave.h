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
#include "ESPNowCommunicator.h"
#include "Message.h"
#include "UARTCommunicator.h"


enum SlaveCTREnum {

#if CONFIG_STR_SLAVE_ESPNOW
	CTR_ESPNOW,
#endif

#if CONFIG_STR_SLAVE_UART0
	CTR_UART0,
#endif

#if CONFIG_STR_SLAVE_UART1
	CTR_UART1,
#endif

#if CONFIG_STR_SLAVE_UART2
	CTR_UART2,
#endif

	SLAVE_CTR_MAX
};


class Slave
{
    public:

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
		return fCTR[type].has_value();
	}

	void		InitCTR(ICTR_t&& ctr, SlaveCTREnum type) {
		fCTR[type].emplace(std::move(ctr));
	}

#if CONFIG_STR_SLAVE_ESPNOW
	ESPNowCTR&	GetESPNowCTR() { return std::get<ESPNowCTR>(*fCTR[CTR_ESPNOW]); }
#endif

	void		SetWaitingForID(const bool value = true) {
		fWaitingForID = value;
	}

	bool		IsWaitingForID() const {
		return fWaitingForID;
	}

	void		Write() const {
		const auto& ctrToUse = fCTR[fCTRToUse];

		if (ctrToUse)
			ICTR_T_WRITE(*ctrToUse, , );
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
	
	std::array<std::optional<ICTR_t>, SlaveCTREnum::SLAVE_CTR_MAX>      fCTR;
	uint8_t																fCTRToUse = 0;

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
