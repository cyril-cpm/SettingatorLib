#include "CommunicatorBridge.h"
#include "Communicator.h"
#include "Message.h"
#include "ESPNowCommunicator.h"

#if defined(ARDUINO)
#include <WiFi.h>

#elif defined(ESP_PLATFORM)
#include <stdlib.h>
#include <cstring>
#include <esp_log.h>
#include "esp_task_wdt.h"

#endif

CTRBridge* CTRBridge::CreateInstance(ICTR* master)
{
    ESP_LOGI("CTRBridge", "Creating Instance");
    return new CTRBridge(master);
}

CTRBridge::CTRBridge(ICTR* master)
{
    if (master)
        masterCTR = master;
    ESP_LOGI("CTRBridge", "Instance created");
}

void CTRBridge::SetMaster(ICTR* master)
{
	if (master)
		masterCTR = master;
}

void CTRBridge::ShouldSendLinkInfo(bool should)
{
	fShouldSendLinkInfo = should;
}

void linkInfoCallback(TimerHandle_t timer)
{
	BRIDGE.ShouldSendLinkInfo();
}

void    CTRBridge::CreateLinkInfoTimer()
{
    if (!fLinkInfoTimer)
    {
        fLinkInfoTimer = xTimerCreate(
            "LinkInfoTImer",
            pdMS_TO_TICKS(5000),
            pdTRUE,
            (void*)0,
            linkInfoCallback
        );

        xTimerStart(fLinkInfoTimer, 0);
    }
}

void CTRBridge::begin()
{
#if defined(ESP_PLATFORM)
	if (esp_task_wdt_status(nullptr) == ESP_ERR_NOT_FOUND)
    	ESP_ERROR_CHECK(esp_task_wdt_add(nullptr));

	CreateLinkInfoTimer();
#endif
}

void CTRBridge::Update()
{
    // LECTURE DES MESSAGES DU MASTER //
    if (masterCTR && masterCTR->Available())
    {
        Message* msg = masterCTR->Read();
        ESP_LOGI("CRTBridge", "masterCTR data availlable");
        if (msg)
        {
#if defined(ARDUINO)
            Serial.println("A message has been read");
#elif defined(ESP_PLATFORM)
            ESP_LOGI("CRTBridge", "A message has been read");
#endif
            switch (msg->GetType())
            {
            case Message::Type::EspNowStartInitBroadcastedSlave:
                StartEspNowInitBroadcasted();
                break;

            case Message::Type::EspNowStopInitBroadcastedSlave:
                StopEspNowInitBroadcasted();
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

            case Message::Type::BridgeReinitSlaves:
                _reinitSlaves();
                break;

            default:
                if (msg->GetType() < Message::Type::BridgeBase)
                {
                    ICTR* slaveCTR = Slave::GetSlaveCTR(msg->GetSlaveID());

                    if (slaveCTR)
                    {
                        slaveCTR->Write(*msg);
                    }

                    else if (msg->GetType() == Message::Type::InitRequest && !slavesWaitingForID.empty())
                    {
                        Slave* slave = slavesWaitingForID.front();
                        if (slave)
                        {
                            if (slave->GetID() == 0)
                            {
                                slave->SetID(msg->GetSlaveID());
                                slaves.push_back(slave);
                            }
                            else
                                slave->AddSubSlave(msg->GetSlaveID());
                            slavesWaitingForID.pop();
                            slaveCTR = slave->GetCTR();

                            if (slaveCTR)
                                slaveCTR->Write(*msg);
                        }
                    }
                }
                break;
            }
        }
        masterCTR->Flush();
    }

    // TRAITEMENT DES SLAVES //
    if (masterCTR)
    {
        for (auto i = slaves.begin(); i != slaves.end(); i++)
        {
            ICTR* slaveCTR = (*i)->GetCTR();

            // LECTURE DES MESSAGES DES SLAVES //
            if (slaveCTR && slaveCTR->Available())
            {
                Message* msg = slaveCTR->Read();

                if (msg)
                {
                    switch (msg->GetType())
                        {
                        case Message::Type::SettingInit:
                            _treatSettingInit(msg, *i);
                            break;

                        case Message::Type::SlaveIDRequest:
                            slavesWaitingForID.push(*i);
                            break;
                        default:
                        break;
                        }
                    masterCTR->Write(*msg);
                }
                slaveCTR->Flush();
            }
        }
    }

    //ESP_LOGI("CTRBridge", "slaves done");

    // TRAITEMENT DES NOUVEAUX SLAVES (assignation ID et initRequest) //
    if (masterCTR && !newSlavesCTR.empty())
    {
        Message* requestMsg = Message::BuildSlaveIDRequestMessage();

        while (!newSlavesCTR.empty())
        {
            ICTR* ctr = newSlavesCTR.front();

            bool ctrIsUsed = false;
            uint8_t slaveID = 0;

            for (auto i = slaves.begin(); i != slaves.end(); i++)
            {
                if ((*i)->GetCTR() == ctr)
                {
                    ctrIsUsed = true;
                    slaveID = (*i)->GetID();
                    break;
                }
            }

            if (ctrIsUsed)
            {
                Message* initRequestMsg = Message::BuildInitRequestMessage(slaveID);
                ctr->Write(*initRequestMsg);
                delete initRequestMsg;
            }
            else
            {
                masterCTR->Write(*requestMsg);
                Slave* newSlave = new Slave(newSlavesCTR.front());
                slavesWaitingForID.push(newSlave);
            }
            newSlavesCTR.pop();
        }

        delete requestMsg;
    }

    //ESP_LOGI("CTRBridge", "new CTR done");
    
#if defined(ESP_PLATFORM)
	HandleLinkInfo();

	ESP_ERROR_CHECK(esp_task_wdt_reset());
    vTaskDelay(1);
#endif
}

void CTRBridge::StartEspNowInitBroadcasted()
{
    ESPNowCore::CreateInstance();
    initEspNowBroadcasted = true;
    ESP_LOGI("CTRBridge", "start esp now init broadcasted slaves");
}

void CTRBridge::StopEspNowInitBroadcasted()
{
    initEspNowBroadcasted = false;
    ESP_LOGI("CTRBridge", "stop esp now init broadcasted slaves");
}

void CTRBridge::_configDirectNotif(Message* msg)
{
    /*if (!msg)
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

    if (Slave::GetSlaveCTR(srcSlaveID))
        Slave::GetSlaveCTR(srcSlaveID)->Write(*configMsg);

    free(configBuffer);
    delete configMsg;*/
}

void CTRBridge::_configDirectSettingUpdate(Message* msg)
{
    /*if (!msg)
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

    if (Slave::GetSlaveCTR(srcSlaveID))
        Slave::GetSlaveCTR(srcSlaveID)->Write(*configMsg);

    free(configBuffer);
    delete configMsg;*/
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

    if (Slave::GetSlaveCTR(srcSlaveID))
        Slave::GetSlaveCTR(srcSlaveID)->Write(*configMsg);

    free(configBuffer);
    delete configMsg;
}

void CTRBridge::_reinitSlaves()
{
    //Serial.println("_reinitSLaves");
    for (auto i = slaves.begin(); i != slaves.end(); i++)
    {
        //Serial.println("looping");
        ICTR* slaveCTR = (*i)->GetCTR();

        if (slaveCTR)
        {
            Message* msg =  Message::BuildInitRequestMessage((*i)->GetID());

            if (msg)
            {
                slaveCTR->Write(*msg);
                //Serial.println("Slave reinit");

                delete msg;
            }

            Message* reinitMsg = Message::BuildReInitSlaveMessage();

            if (reinitMsg)
            {
                slaveCTR->Write(*reinitMsg);
                delete msg;
            }
        }
    }    
}

void CTRBridge::_treatSettingInit(Message* msg, Slave *slave)
{
    if (!msg || !slave)
        return;

    uint8_t msgSlaveID = msg->GetSlaveID();

    if (msgSlaveID == slave->GetID())
        return;

    if (!slave->HasSubSlave(msgSlaveID))
        slave->AddSubSlave(msgSlaveID);
}

void CTRBridge::HandleLinkInfo()
{
	if (!fShouldSendLinkInfo || !espNowCore || !masterCTR)
		return;

	uint8_t nbCTR = 0;

	uint16_t msgSize = 13;
	for (auto i = slaves.begin(); i != slaves.end(); i++)
	{
		if (*i)
		{
			msgSize += (*i)->GetLinkInfoSize();
			nbCTR++;
		}
	}

	uint8_t msgBuffer[msgSize];
	
	msgBuffer[0] = Message::Frame::Start;
	msgBuffer[1] = msgSize >> 8;
	msgBuffer[2] = msgSize;
	msgBuffer[3] = 0;
	msgBuffer[4] = Message::Type::LinkInfo;
	msgBuffer[5] = nbCTR;

	memcpy(msgBuffer + 6, espNowCore->GetMac(), 6);

	uint16_t bufIndex = 12;
	for (auto i = slaves.begin(); i != slaves.end(); i++)
	{
		if (*i)
		{
			(*i)->WriteLinkInfoToBuffer(msgBuffer + bufIndex);
			bufIndex += (*i)->GetLinkInfoSize();
		}
	}

	msgBuffer[bufIndex - 1] = Message::Frame::End;

	Message msg(msgBuffer, msgSize);
	masterCTR->Write(msg);

	fShouldSendLinkInfo = false;
}
