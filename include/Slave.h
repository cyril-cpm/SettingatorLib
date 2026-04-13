#pragma once

#include "Definitions.h"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <variant>
#include "Communicator.h"
#include "ESPNowCommunicator.h"
#include "UARTCommunicator.h"

using ICTR_t = std::variant<std::monostate
#if STR_HAS_UART
	, UARTCTR
#endif

#if STR_HAS_ESPNOW
	, ESPNowCTR
#endif
>;

#define NOT_MONOSTATE_CHECK(X, Y, Z) using T = std::decay_t<decltype(X)>; \
		if constexpr (!std::is_same_v<T, std::monostate>) \
		Y \
		Z

#define ICTR_T_AVAILABLE(X) std::visit([](auto&& ctr) -> bool { \
		NOT_MONOSTATE_CHECK(ctr, return ctr.Available(); , return false;) \
	}, X)

#define ICTR_T_READ(X) std::visit([](auto&& ctr) -> Message* { \
		NOT_MONOSTATE_CHECK(ctr, return ctr.Read(); , return nullptr;) \
	}, X)

#define ICTR_T_FLUSH(X) std::visit([](auto&& ctr) { \
		NOT_MONOSTATE_CHECK(ctr, ctr.Flush(); ,) \
	}, X)

#define ICTR_T_WRITE(CTR, CAPT, MSG) std::visit([CAPT](auto&& ctr) -> int { \
		NOT_MONOSTATE_CHECK(ctr, return ctr.Write(MSG); , return 0;) \
	}, CTR)

#define ICTR_T_GET_PTR(CTR) std::visit([](auto&& ctr) -> ICTR* { \
		NOT_MONOSTATE_CHECK(ctr, return &ctr; , return nullptr); \
	}, CTR)

#define IS_TYPE_CHECK(CTR, TYPE, OK, KO) using T = std::decay_t<decltype(CTR)>; \
		if constexpr (std::is_same_v<T, TYPE>) \
		OK \
		KO

#define ESPNOWCTR_GET_MAC(CTR) std::visit([](ESPNowCTR&& ctr) -> const std::array<uint8_t, 6>& { \
		return ctr.GetMac(); \
	}, CTR)

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

    ICTR_t*		GetCTR();
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

    private:
    uint8_t     			fSlaveID = 0;
	
	std::array<std::optional<ICTR_t>, SlaveCTREnum::SLAVE_CTR_MAX>      fCTR;
	uint8_t																fCTRToUse = 0;

	bool					fWaitingForID = false;
	std::array<uint8_t, 6>	fEMac;

#if CONFIG_STR_NB_SUBSLAVE
    std::array<uint8_t, CONFIG_STR_NB_SUBSLAVE> fSubSlave;
	uint8_t										fSubSlaveCount = 0;
#endif
};

using OptSlaveRef = std::optional<std::reference_wrapper<Slave>>;

extern std::queue<Slave*> reconnectedSlaves;

extern bool initEspNowBroadcasted;
