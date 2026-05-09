#include "CommunicatorBridge.h"
#include "Master.h"
#include "UARTCommunicator.h"
#include "esp_attr.h"
#include "esp_log_buffer.h"

#if CONFIG_STR_HAS_BRIDGE

#include "Communicator.h"
#include "STR.h"
#include "Slave.h"
#include "Message.h"
#include "ESPNowCommunicator.h"
#include <cstdint>
#include <type_traits>
#include <variant>
#include "MiscDef.h"
#include <stdlib.h>
#include <cstring>
#include <esp_log.h>
#include "esp_task_wdt.h"

static const char* tag("CTRBridge");

static Master& master = Master::GetInstance();

void CTRBridge::ShouldSendLinkInfo(bool should)
{
	fShouldSendLinkInfo = should;
}

void linkInfoCallback(TimerHandle_t timer)
{
	CTRBridge::GetInstance().ShouldSendLinkInfo();
}

void	CTRBridge::CreateLinkInfoTimer()
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
	if (esp_task_wdt_status(nullptr) == ESP_ERR_NOT_FOUND)
		ESP_ERROR_CHECK(esp_task_wdt_add(nullptr));

	CreateLinkInfoTimer();

#if CONFIG_STR_SLAVE_ESPNOW
	ESPNowCore::GetInstance().BroadcastBridgePing();
#endif

}

void CTRBridge::Update()
{
	for (ICore& core : coreArray)
	{
		if (core.FetchMessage())
		{
			LOG("MSG FETCHED");
			if (core.GetSrcSlaveID() == 0)
			{
				switch (core.GetMessageType())
				{
				case Message::Type::EspNowStartInitBroadcastedSlave:
					LOG("START ESPNOW StartEspNowInitBroadcasted");
					StartEspNowInitBroadcasted();
					break;

				case Message::Type::EspNowStopInitBroadcastedSlave:
					StopEspNowInitBroadcasted();
					break;

				case Message::Type::EspNowConfigDirectNotif:
					// _configDirectNotif(*msg);
					break;

				case Message::Type::EspNowConfigDirectSettingUpdate:
					// _configDirectSettingUpdate(*msg);
					break;

				case Message::Type::EspNowRemoveDirectNotifConfig:
					// _removeDirectMessageConfig(*msg, Message::Type::RemoveDirectNotifConfig);
					break;

				case Message::Type::EspNowRemoveDirectSettingUpdateConfig:
					// _removeDirectMessageConfig(*msg, Message::Type::RemoveDirectSettingUpdateConfig);
					break;

				case Message::Type::BridgeReinitSlaves:
					_reinitSlaves();
					break;

				default:
					if (core.GetMessageType() < Message::Type::BridgeBase)
					{
						if (core.GetMessageType() == Message::Type::InitRequest)
						{
							LOG("InitRequest");
							
							for (auto& slave : slaveArray)
							{
								if (!slave)
									break;

								if (slave.IsWaitingForID())
								{
									LOG("Setting id %d to slave", core.GetDstSlaveID());
									slave.SetID(core.GetDstSlaveID());
									slave.SetWaitingForID(false);
									break;
								}
							}
						}
						const auto& slave = GetSlaveForID(core.GetDstSlaveID());

						if (slave)
						{
							LOG("transmitting msg to slave %d", slave->get().GetID());
							ESP_LOG_BUFFER_HEX("CTRBridge", slave->get().GetEMac().data(),
												slave->get().GetEMac().size());
							core.CopyMessageToGlobalBuffer();
							slave->get().Write();
						}
					}
					break;
				}
			} // srcID == 0
			else
			{
				if (core.GetMessageType() != Message::Type::EspNowPong)
				{
					LOG("transmitting message to master");
					// LOG_BUFFER_HEX(msg->GetBufPtr(), msg->GetLength());
					core.CopyMessageToGlobalBuffer();
					master.Write();
				}
			}
			core.ThrowMessage();
		}
	}

	for (auto& slave : slaveArray)
	{
		if (!slave)
			break;
		
		if (!slave.GetID() && !slave.IsWaitingForID())
		{
			LOG("Slave without ID found, requesting one");
			slave.SetWaitingForID(true);
			// Send ID request trhough master
			master.Write({
							Message::Frame::Start,
							0x00,
							0x07,
							0,
							0,
							Message::Type::SlaveIDRequest,
							Message::Frame::End
						});
		}
		
		// slave.Update();
		slave.HandleSendInitRequest();
	}

	HandleLinkInfo();

#if CONFIG_STR_UART0
	UARTCore::GetUART0Instance().Read();
#endif

#if CONFIG_STR_UART1
	UARTCore::GetUART1Instance().Read();
#endif

#if CONFIG_STR_UART2
	UARTCore::GetUART2Instance().Read();
#endif

	ESP_ERROR_CHECK(esp_task_wdt_reset());
	vTaskDelay(1);
}

// void CTRBridge::_Update()
// {
// 	// TRAITEMENT DES SLAVES //
// 	if (masterCTR.index())
// 	{
// 		for (auto& slave : slaves)
// 		{
// 			ICTR_t* slaveCTR = slave.GetCTR();
//
// 			// LECTURE DES MESSAGES DES SLAVES //
// 			if (slaveCTR && ICTR_T_AVAILABLE(*slaveCTR))
// 			{
// 				Message* msg = ICTR_T_READ(*slaveCTR);
//
// 				if (msg)
// 				{
// 					switch (msg->GetType())
// 						{
// 						case Message::Type::SettingInit:
// 							_treatSettingInit(*msg, slave);
// 							break;
//
// 						case Message::Type::SlaveIDRequest:
// 							// slavesWaitingForID.push(slave); looks broken
// 							break;
// 						default:
// 						break;
// 						}
// 					if (msg->GetType() != Message::Type::EspNowPong)
// 					{
// 						LOG("transmitting message to maste");
// 						// LOG_BUFFER_HEX(msg->GetBufPtr(), msg->GetLength());
// 						// ICTR_T_WRITE(masterCTR, msg, *msg);
// 					}
// 				}
// 				ICTR_T_FLUSH(*slaveCTR);
// 			}
// 		}
// 	}
//
//
// 	// TRAITEMENT DES NOUVEAUX SLAVES (assignation ID et initRequest) //
// 	if (masterCTR.index())
// 	{
// 		newSlavesCTRMutex.lock();
// 		while (!newSlavesCTR.empty())
// 		{
// 			ICTR_t ctr = std::move(newSlavesCTR.front());
// 			newSlavesCTR.pop();
//
// 			ICTR_T_WRITE(masterCTR, , Message::BuildSlaveIDRequestMessage());
//
// 			slavesWaitingForID.emplace(Slave(std::move(ctr)));
// 		}
// 		newSlavesCTRMutex.unlock();
//
//
//
//
// 		while (!reconnectedSlaves.empty())
// 		{
// 			LOG("A SlaveCTR has reconnected");
// 			Slave* slave = reconnectedSlaves.front();
// 			reconnectedSlaves.pop();
//
// 			ICTR_T_WRITE(*(slave->GetCTR()), slave, Message::BuildInitRequestMessage(slave->GetID()));
// 		}
// 	}
//
//
// #if defined(ESP_PLATFORM)
// 	HandleLinkInfo();
//
// 	ESP_ERROR_CHECK(esp_task_wdt_reset());
// 	vTaskDelay(1);
// #endif
// }

void CTRBridge::StartEspNowInitBroadcasted()
{
#if STR_HAS_ESPNOW
	LOG("StartEspNowInitBroadcasted");
	ESPNowCore::GetInstance();
	initEspNowBroadcasted = true;
#endif
}

void CTRBridge::StopEspNowInitBroadcasted()
{
	initEspNowBroadcasted = false;
}

void CTRBridge::_configDirectNotif(Message& msg)
{
	/*if (!msg)
		return;

	auto srcSlaveID = msg->GetSlaveID();
	auto dstSalveID = msg->GetBufPtr()[4];
	auto notifByte = msg->GetBufPtr()[5];

	uint16_t configBufferLength = 14;

	uint8_t* configBuffer = (uint8_t*)mlalloc(sizeof(uint8_t) * configBufferLength);

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

	Message configMsg = Message(configBuffer, configBufferLength);

	if (Slave::GetSlaveCTR(srcSlaveID))
		Slave::GetSlaveCTR(srcSlaveID)->Write(*configMsg);

	free(configBuffer);
	delete configMsg;*/
}

void CTRBridge::_configDirectSettingUpdate(Message& msg)
{
	/*if (!msg)
		return;

	auto srcSlaveID = msg->GetSlaveID();
	auto dstSalveID = msg->GetBufPtr()[5];
	auto settingRef = msg->GetBufPtr()[6];

	uint16_t configBufferLength = 15;

	uint8_t* configBuffer = (uint8_t*)mlalloc(sizeof(uint8_t) * configBufferLength);

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

	Message configMsg = Message(configBuffer, configBufferLength);

	if (Slave::GetSlaveCTR(srcSlaveID))
		Slave::GetSlaveCTR(srcSlaveID)->Write(*configMsg);

	free(configBuffer);
	delete configMsg;*/
}

void CTRBridge::_removeDirectMessageConfig(Message& msg, uint8_t messageType)
{
	// if (Slave::GetSlaveCTR(msg.GetSlaveID()))
	// 	ICTR_T_WRITE(*Slave::GetSlaveCTR(msg.GetSlaveID()), &msg, msg);
}

void CTRBridge::_reinitSlaves()
{
	//Serial.println("_reinitSLaves");
	// for (auto& slave : slaveArray)
	// {
	// 	//Serial.println("looping");
	// 	ICTR_t* slaveCTR = slave.GetCTR();
	//
	// 	if (slaveCTR)
	// 	{
	// 		ICTR_T_WRITE(*slaveCTR, &slave, Message::BuildInitRequestMessage(slave->GetID()));
	// 		ICTR_T_WRITE(*slaveCTR,,Message::BuildReInitSlaveMessage());
	// 	}
	// }
}

void CTRBridge::_treatSettingInit(Message& msg, Slave& slave)
{
	// LOG("Init Setting from slave %d:", msg.GetSlaveID());
	// uint8_t msgSlaveID = msg.GetSlaveID();
	//
	// if (msgSlaveID == slave.GetID())
	// 	return;
	//
	// if (!slave.HasSubSlave(msgSlaveID))
	// 	slave.AddSubSlave(msgSlaveID);
}

void CTRBridge::HandleLinkInfo()
{
	if (!fShouldSendLinkInfo)
		return;

	uint8_t nbCTR = 0;

	uint16_t msgSize = 14;
	if (msgSize > CONFIG_STR_MESSAGE_BUFFER_SIZE)
	{
		LOG("LinkInfoSize > CONFIG_STR_MESSAGE_BUFFER_SIZE");
		return;
	}

	for (auto& slave : slaveArray)
	{
		if (!slave)
			break;
		if (msgSize + slave.GetLinkInfoSize() > CONFIG_STR_MESSAGE_BUFFER_SIZE)
		{
			LOG("LinkInfoSize > CONFIG_STR_MESSAGE_BUFFER_SIZE");
			return;
		}
		slave.WriteLinkInfoToBuffer(msgSize - 1);
		msgSize += slave.GetLinkInfoSize();
		nbCTR++;
	}

	messageBuffer[0] = Message::Frame::Start;
	messageBuffer[1] = msgSize >> 8;
	messageBuffer[2] = msgSize;
	messageBuffer[3] = 0;
	messageBuffer[4] = 0;
	messageBuffer[5] = Message::Type::LinkInfo;
	messageBuffer[6] = nbCTR;
	memcpy(messageBuffer.data() + 7, ESPNowCore::GetInstance().GetMac().data(), 6);
	messageBuffer[msgSize-1] = Message::Frame::End;
	messageBuffer.SetLen(msgSize);

	master.Write();

	fShouldSendLinkInfo = false;
}

#endif
