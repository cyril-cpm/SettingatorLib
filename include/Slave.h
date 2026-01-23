#pragma once

#include "ESPNowCommunicator.h"
#include "UARTCommunicator.h"

using ICTR_t = std::variant<std::monostate, UARTCTR, ESPNowCTR>;

extern std::queue<ICTR_t> newSlavesCTR;

class Slave
{
    public:
    Slave(auto ctr);

    static ICTR* GetSlaveCTR(uint8_t slaveID);

    ICTR*		GetCTR();
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

