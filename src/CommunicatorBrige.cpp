#include "CommunicatorBridge.h"
#include "Communicator.h"
#include "Message.h"
#include "ESPNowCommunicator.h"

#include <WiFi.h>

CTRBridge* CTRBridge::CreateInstance(ICTR* master)
{
    return new CTRBridge(master);
}

CTRBridge::CTRBridge(ICTR* master) : fMaster(master)
{}

void CTRBridge::Update()
{
    if (fMaster->Available())
    {
        Message* msg = fMaster->Read();

        if (msg)
        {
            if (msg->GetType() == Message::Type::EspNowInitWithSSD)
            {
                _addEspNowSlaveWithSSD(msg->ExtractSSD(), msg->GetSlaveID());
            }
            else if (msg->GetType() < Message::Type::BridgeBase)
            {
                ICTR* slaveCTR = _getSlaveCTR(msg->GetSlaveID());

                if (slaveCTR)
                    slaveCTR->Write(*msg);
            }
        }
        fMaster->Flush();
    }

    for (auto i = fSlaves.begin(); i != fSlaves.end(); i++)
    {
        if (i->fCTR && i->fCTR->Available())
        {
            Message* msg = i->fCTR->Read();

            if (msg)
                fMaster->Write(*msg);
            i->fCTR->Flush();
        }
    }
}

void CTRBridge::_addEspNowSlaveWithSSD(char* SSID, uint8_t slaveID)
{
    uint16_t numberOfNetwork = WiFi.scanNetworks(false, false, false, 300U, 0U, SSID);

    for (int i = 0; i < numberOfNetwork; i++)
        _addEspNowSlaveWithMac(WiFi.BSSID(i), slaveID + i);

}

void CTRBridge::_addEspNowSlaveWithMac(uint8_t* Mac, uint8_t slaveID)
{
     fSlaves.push_back(slave(ESPNowCTR::CreateInstanceWithMac(Mac), slaveID, Mac));
}

ICTR* CTRBridge::_getSlaveCTR(uint8_t slaveID)
{
    for (auto i = fSlaves.begin(); i != fSlaves.end(); i++)
    {
        if (i->fSlaveID == slaveID)
            return i->fCTR;
    }

    return nullptr;
}

slave::slave(ICTR* ctr, uint8_t slaveID, uint8_t* mac) : fCTR(ctr), fSlaveID(slaveID), fMac(mac)
{
    Message* msg = Message::BuildInitRequestMessage(slaveID);
    fCTR->Write(*msg);
    delete msg;
}