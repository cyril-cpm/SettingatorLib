#pragma once

#include "Message.h"

#include <vector>

class ICTR;

struct slave
{
    slave(ICTR* ctr, uint8_t slaveID, uint8_t* mac = nullptr);

    uint8_t*    fMac = nullptr;
    uint8_t     fSlaveID = 0;
    ICTR*       fCTR = nullptr;
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
    void    _configDirectNotif(Message* msg);
    void    _configDirectSettingUpdate(Message* msg);

    ICTR*   _getSlaveCTR(uint8_t slaveID);
    uint8_t*   _getSlaveMac(uint8_t slaveID);

    ICTR*               fMaster = nullptr;
    std::vector<slave>  fSlaves;
};