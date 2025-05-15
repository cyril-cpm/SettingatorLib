#include "CommunicatorBridge.h"
#include "Communicator.h"
#include "Message.h"
#include "ESPNowCommunicator.h"

#if defined(ARDUINO)
#include <WiFi.h>

#elif defined(ESP_PLATFORM)
#include <stdlib.h>
#include <cstring>

#endif

CTRBridge* CTRBridge::CreateInstance(ICTR* master)
{
    return new CTRBridge(master);
}

CTRBridge::CTRBridge(ICTR* master)
{
    fMaster = master;
}

void CTRBridge::Update()
{
    if (fMaster && fMaster->Available())
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

            case Message::Type::EspNowRemoveDirectNotifConfig:
                _removeDirectMessageConfig(msg, Message::Type::RemoveDirectNotifConfig);
                break;

            case Message::Type::EspNowRemoveDirectSettingUpdateConfig:
                _removeDirectMessageConfig(msg, Message::Type::RemoveDirectSettingUpdateConfig);
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

#if defined(ARDUINO)
    uint16_t numberOfNetwork = WiFi.scanNetworks(false, false, false, 300U, 0U, SSID);

    for (int i = 0; i < numberOfNetwork; i++)
        _addEspNowSlaveWithMac(WiFi.BSSID(i), slaveID + i);

#elif defined(ESP_PLATFORM)

#endif

}

void CTRBridge::_addEspNowSlaveWithMac(uint8_t* Mac, uint8_t slaveID)
{
     fSlaves.push_back(slave(ESPNowCTR::CreateInstanceWithMac(Mac), slaveID, Mac));
}

void CTRBridge::_configDirectNotif(Message* msg)
{
    if (!msg)
        return;

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

    if (_getSlaveCTR(srcSlaveID))
        _getSlaveCTR(srcSlaveID)->Write(*configMsg);

    free(configBuffer);
    delete configMsg;
}

void CTRBridge::_configDirectSettingUpdate(Message* msg)
{
    if (!msg)
        return;

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

    if (_getSlaveCTR(srcSlaveID))
        _getSlaveCTR(srcSlaveID)->Write(*configMsg);

    free(configBuffer);
    delete configMsg;
}

void CTRBridge::_removeDirectMessageConfig(Message* msg, uint8_t messageType)
{
    uint8_t* buffer = nullptr;

    if (!msg && !(buffer = msg->GetBufPtr()))
        return;

    uint8_t srcSlaveID = msg->GetSlaveID();
    uint8_t dstSlaveID = buffer[5];
    uint8_t configID = buffer[6];

    uint16_t configBufferLength = 8;

    uint8_t* configBuffer = (uint8_t*) malloc(sizeof(uint8_t) * configBufferLength);

    configBuffer[0] = Message::Frame::Start;
    configBuffer[1] = 0;
    configBuffer[2] = configBufferLength;
    configBuffer[3] = srcSlaveID;
    configBuffer[4] = messageType;
    configBuffer[5] = dstSlaveID;
    configBuffer[6] = configID;
    configBuffer[7] = Message::Frame::End;

    Message* configMsg = new Message(configBuffer, configBufferLength);

    if (_getSlaveCTR(srcSlaveID))
        _getSlaveCTR(srcSlaveID)->Write(*configMsg);

    free(configBuffer);
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

slave::slave(ICTR* ctr, uint8_t slaveID, uint8_t* mac)
{
    fCTR = ctr;
    fSlaveID = slaveID;
    fMac = mac;

    Message* msg = Message::BuildInitRequestMessage(slaveID);

    if (fCTR)
        fCTR->Write(*msg);
    delete msg;
}