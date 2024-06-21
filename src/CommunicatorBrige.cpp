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
            switch (msg->GetType())
            {
            case Message::Type::EspNowInitWithSSD:
                _addEspNowSlaveWithSSD(msg->ExtractSSD(), msg->GetSlaveID());
                break;
            
            case Message::Type::EspNowConfigDirectNotif:
                _configDirectNotif(msg);
                break;
            case Message::Type::EspNowConfigDirectSettingUpdate:
                _configDirectSettingUpdate(msg);
                break;
            default:
                if (msg->GetType() < Message::Type::BridgeBase)
                {
                    ICTR* slaveCTR = _getSlaveCTR(msg->GetSlaveID());

                    if (slaveCTR)
                        slaveCTR->Write(*msg);
                }
                break;
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

void CTRBridge::_configDirectNotif(Message* msg)
{
    auto srcSlaveID = msg->GetSlaveID();
    auto dstSalveID = msg->GetBufPtr()[4];
    auto notifByte = msg->GetBufPtr()[5];

    uint16_t configBufferLength = 14;

    uint8_t* configBuffer = (uint8_t*)malloc(sizeof(uint8_t) * configBufferLength);

    configBuffer[0] = Message::Frame::Start;
    configBuffer[1] = 0;
    configBuffer[2] = configBufferLength;
    configBuffer[3] = srcSlaveID;
    configBuffer[4] = Message::Type::ConfigEspNowDirectNotif;
    configBuffer[5] = dstSalveID;

    uint8_t* dstMac = _getSlaveMac(dstSalveID);

    if (dstMac)
        memcpy(&configBuffer[6], dstMac, 6);
    else
        bzero(&configBuffer[6], 6);

    configBuffer[12] = notifByte;
    configBuffer[13] = Message::Frame::End;

    Message* configMsg = new Message(configBuffer, configBufferLength);

    _getSlaveCTR(srcSlaveID)->Write(*configMsg);

    delete configBuffer;
    delete configMsg;
}

void CTRBridge::_configDirectSettingUpdate(Message* msg)
{
    auto srcSlaveID = msg->GetSlaveID();
    auto dstSalveID = msg->GetBufPtr()[5];
    auto settingRef = msg->GetBufPtr()[6];

    uint16_t configBufferLength = 15;

    uint8_t* configBuffer = (uint8_t*)malloc(sizeof(uint8_t) * configBufferLength);

    configBuffer[0] = Message::Frame::Start;
    configBuffer[1] = 0;
    configBuffer[2] = configBufferLength;
    configBuffer[3] = srcSlaveID;
    configBuffer[4] = Message::Type::ConfigEspNowDirectSettingUpdate;
    configBuffer[5] = dstSalveID;

    uint8_t* dstMac = _getSlaveMac(dstSalveID);

    if (dstMac)
        memcpy(&configBuffer[6], dstMac, 6);
    else
        bzero(&configBuffer[6], 6);

    configBuffer[12] = settingRef;
    configBuffer[13] = msg->GetBufPtr()[7];
    configBuffer[14] = Message::Frame::End;

    Message* configMsg = new Message(configBuffer, configBufferLength);

    _getSlaveCTR(srcSlaveID)->Write(*configMsg);

    delete configBuffer;
    delete configMsg;
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

uint8_t* CTRBridge::_getSlaveMac(uint8_t slaveID)
{
    for (auto i = fSlaves.begin(); i != fSlaves.end(); i++)
    {
        if (i->fSlaveID == slaveID)
            return i->fMac;
    }

    return nullptr;
}

slave::slave(ICTR* ctr, uint8_t slaveID, uint8_t* mac) : fCTR(ctr), fSlaveID(slaveID), fMac(mac)
{
    Message* msg = Message::BuildInitRequestMessage(slaveID);
    fCTR->Write(*msg);
    delete msg;
}