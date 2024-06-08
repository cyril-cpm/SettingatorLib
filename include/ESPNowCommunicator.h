#pragma once

#include "Communicator.h"

class Message;

struct espNowMsg {
    espNowMsg(const uint8_t* inData, int inLen);
    uint8_t*    data = nullptr;
    int         len = 0;
};

extern std::queue<espNowMsg>    espNowMsgList;
    
class ESPNowCTR: public ICTR
{
    public:

    static ESPNowCTR*   CreateInstance(uint8_t* mac = nullptr);

    virtual int         Write(Message& buf) override;
    virtual void        Update() override;

    private:
    ESPNowCTR(uint8_t* mac = nullptr);
    ~ESPNowCTR();

    uint8_t* fMac = nullptr;
};