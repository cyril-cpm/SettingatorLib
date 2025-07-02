#pragma once

#include "Message.h"

#include <vector>

class ICTR;
class Slave;

class CTRBridge
{
    public:

    static CTRBridge*   CreateInstance(ICTR* master);

    CTRBridge(ICTR* first);

    void                Update();

    private:

    void    _addEspNowSlaveWithSSD(char* SSD, uint8_t slaveID);
    void    _startEspNowInitBroadcasted();
    void    _stopEspNowInitBroadcasted();
    void    _configDirectNotif(Message* msg);
    void    _configDirectSettingUpdate(Message* msg);
    void    _removeDirectMessageConfig(Message* msg, uint8_t messageType);
    void    _reinitSlaves();
    void    _treatSettingInit(Message* msg, Slave* slave);
};