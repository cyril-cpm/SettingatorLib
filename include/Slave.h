#pragma once

#include <variant>
#include "Communicator.h"
#include "ESPNowCommunicator.h"
#include "UARTCommunicator.h"

using ICTR_t = std::variant<std::monostate, UARTCTR, ESPNowCTR>;

#define MONOSTATE_CHECK(X, Y, Z) using T = std::decay_t<decltype(X)>; \
		if constexpr (!std::is_same_v<T, std::monostate>) \
		Y \
		Z

#define ICTR_T_AVAILABLE(X) std::visit([](auto&& ctr) -> bool { \
		MONOSTATE_CHECK(ctr, return ctr.Available(); , return false;) \
	}, X)

#define ICTR_T_READ(X) std::visit([](auto&& ctr) -> Message* { \
		MONOSTATE_CHECK(ctr, return ctr.Read(); , return nullptr;) \
	}, X)

#define ICTR_T_FLUSH(X) std::visit([](auto&& ctr) { \
		MONOSTATE_CHECK(ctr, ctr.Flush(); ,) \
	}, X)

#define ICTR_T_WRITE(CTR, CAPT, MSG) std::visit([CAPT](auto&& ctr) -> int { \
		MONOSTATE_CHECK(ctr, return ctr.Write(MSG); , return 0;) \
	}, CTR)

#define ICTR_T_GET_PTR(CTR) std::visit([](auto&& ctr) -> ICTR* { \
		MONOSTATE_CHECK(ctr, return &ctr; , return nullptr); \
	}, CTR)

extern std::queue<ICTR_t> newSlavesCTR;
extern std::queue<ICTR*> reconnectedSlavesCTR;

class Slave
{
    public:
    Slave(ICTR_t&& ctr);

    static ICTR_t* GetSlaveCTR(uint8_t slaveID);

    ICTR_t*		GetCTR();
    uint8_t 	GetID();
    bool    	HasSubSlave(uint8_t id);
    void    	AddSubSlave(uint8_t id);
    void    	SetID(uint8_t id);
	uint16_t	GetLinkInfoSize() const;
	void		WriteLinkInfoToBuffer(uint8_t* msgBuffer) const;

    private:
    uint8_t     fSlaveID = 0;
	ICTR_t      fCTR;

    std::vector<uint8_t> fSubSlave;
};

extern std::vector<Slave*> slaves;
extern std::queue<Slave*> slavesWaitingForID;

extern ICTR_t masterCTR;

extern bool initEspNowBroadcasted;
