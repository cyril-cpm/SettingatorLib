#pragma once

#include "Message.h"

#include <vector>

class ICTR;

struct slave
{
    slave(ICTR* ctr, uint8_t slaveID, uint8_t* mac = nullptr);

    uint8_t*    fMac;
    uint8_t     fSlaveID;
    ICTR*       fCTR;
};

class CTRBridge
{
    public:

    static CTRBridge*   CreateInstance(ICTR* master);

    CTRBridge(ICTR* first);

    void                Update();

    private:

    void    _addEspNowSlaveWithSSD(char* SSD, uint8_t slaveID);
    void    _addEspNowSlaveWithMac(uint8_t* Mac, uint8_t slaveID);

    ICTR*               fMaster = nullptr;
    std::vector<slave>  fSlaves;
};