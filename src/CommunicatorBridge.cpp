#include "CommunicatorBridge.h"
#include "Communicator.h"
#include "Slave.h"
#include "Message.h"
#include "ESPNowCommunicator.h"
#include <type_traits>
#include <variant>

#if defined(ARDUINO)
#include <WiFi.h>

#elif defined(ESP_PLATFORM)
#include <stdlib.h>
#include <cstring>
#include <esp_log.h>
#include "esp_task_wdt.h"

#endif

CTRBridge CTRBridge::CreateInstance(ICTR_t master)
{
	ESP_LOGI("CTRBridge", "Creating Instance");
	return CTRBridge(master);
}

CTRBridge::CTRBridge(ICTR_t master)
{
	if (master.index())
		masterCTR = master;
	ESP_LOGI("CTRBridge", "Instance created");
}

void CTRBridge::SetMaster(ICTR_t master)
{
	if (master.index())
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
#if defined(ESP_PLATFORM)
	if (esp_task_wdt_status(nullptr) == ESP_ERR_NOT_FOUND)
		ESP_ERROR_CHECK(esp_task_wdt_add(nullptr));

	CreateLinkInfoTimer();
#endif
}

void CTRBridge::Update()
{
	// LECTURE DES MESSAGES DU MASTER //
	if (masterCTR.index() && ICTR_T_AVAILABLE(masterCTR))
	{
		Message* msg = ICTR_T_READ(masterCTR);

		ESP_LOGI("CRTBridge", "masterCTR data availlable");
		if (msg)
		{
#if defined(ARDUINO)
			Serial.println("A message has been read");
#elif defined(ESP_PLATFORM)
			ESP_LOGI("CRTBridge", "A message has been read");
			ESP_LOG_BUFFER_HEX("CTRBridge", msg->GetBufPtr(), msg->GetLength());
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
					ICTR_t* slaveCTR = Slave::GetSlaveCTR(msg->GetSlaveID());

					if (slaveCTR)
					{
						std::visit([msg](auto&& ctr) {
								using T = std::decay_t<decltype(ctr)>;

								if constexpr (!std::is_same_v<T, std::monostate>)
									ctr.Write(*msg);
							}, *slaveCTR);
					}

					else if (msg->GetType() == Message::Type::InitRequest && !slavesWaitingForID.empty())
					{
						ESP_LOGI("CTRBridge", "Init Request received");
						Slave* slave = slavesWaitingForID.front();
						if (slave)
						{
							ESP_LOGI("CTRBridge", "Slave waiting for ID found");
							if (slave->GetID() == 0)
							{
								ESP_LOGI("CTRBridge", "Has no ID, push it in slaves vector");
								slave->SetID(msg->GetSlaveID());
								slaves.push_back(slave);
							}
							else
							{
								ESP_LOGI("CTRBridge", "Already has an ID, adding SubSlave");
								slave->AddSubSlave(msg->GetSlaveID());
							}
							slavesWaitingForID.pop();
							slaveCTR = slave->GetCTR();

							if (slaveCTR)
							{
								std::visit([msg](auto&& ctr) {
										using T = std::decay_t<decltype(ctr)>;

										if constexpr (!std::is_same_v<T, std::monostate>)
											ctr.Write(*msg);
									}, *slaveCTR);
							}
						}
					}
				}
				break;
			}
		}
		std::visit([](auto&& ctr) {

				using T = std::decay_t<decltype(ctr)>;

				if constexpr (!std::is_same_v<T, std::monostate>)
					ctr.Flush();
			
			}, masterCTR);

	}

	// TRAITEMENT DES SLAVES //
	if (masterCTR.index())
	{
		for (const auto& slave : slaves)
		{
			ICTR_t* slaveCTR = slave->GetCTR();

			// LECTURE DES MESSAGES DES SLAVES //
			if (slaveCTR && ICTR_T_AVAILABLE(*slaveCTR))
			{
				Message* msg = ICTR_T_READ(*slaveCTR);

				if (msg)
				{
					switch (msg->GetType())
						{
						case Message::Type::SettingInit:
							_treatSettingInit(msg, slave);
							break;

						case Message::Type::SlaveIDRequest:
							slavesWaitingForID.push(slave);
							break;
						default:
						break;
						}
					if (msg->GetType() != Message::Type::EspNowPong)
					{
						std::visit([msg](auto&& ctr) {
								
								using T = std::decay_t<decltype(ctr)>;

								if constexpr (!std::is_same_v<T, std::monostate>)
									ctr.Write(*msg);
							}, masterCTR);
					}
				}
				ICTR_T_FLUSH(*slaveCTR);
			}
		}
	}

	//ESP_LOGI("CTRBridge", "slaves done");

	// TRAITEMENT DES NOUVEAUX SLAVES (assignation ID et initRequest) //
	if (masterCTR.index())
	{
		newSlavesCTRMutex.lock();
		while (!newSlavesCTR.empty())
		{
			ESP_LOGI("CTRBridge", "ewSlavesCTR detected");
			ICTR_t ctr = std::move(newSlavesCTR.front());
			newSlavesCTR.pop();

			bool ctrIsUsed = false;
			uint8_t slaveID = 0;

			std::visit([](auto&& mCtr) {
					using T = std::decay_t<decltype(mCtr)>;

					if constexpr (!std::is_same_v<T, std::monostate>)
						mCtr.Write(Message::BuildSlaveIDRequestMessage());

				}, masterCTR);

			Slave* newSlave = new Slave(std::move(ctr));
			slavesWaitingForID.push(newSlave);
		}
		newSlavesCTRMutex.unlock();




		while (!reconnectedSlavesCTR.empty())
		{
			ICTR* ctr = reconnectedSlavesCTR.front();
			reconnectedSlavesCTR.pop();

			ESP_LOGI("CTRBridge", "A slave has been reconnected");
			if (ctr)
			{
				for (const auto& slave : slaves)
				{
					ESP_LOGI("CTRBridge", "looking for Slave");
					if (slave && ctr == ICTR_T_GET_PTR(*(slave->GetCTR())))
					{
						ESP_LOGI("CTRBridge", "Slave found");
						ICTR_T_WRITE(*(slave->GetCTR()), slave, Message::BuildInitRequestMessage(slave->GetID()));
						break;
					}
				}
			}
		}
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
		ICTR_T_WRITE(*Slave::GetSlaveCTR(srcSlaveID), configMsg, *configMsg);

	free(configBuffer);
	delete configMsg;
}

void CTRBridge::_reinitSlaves()
{
	//Serial.println("_reinitSLaves");
	for (const auto& slave : slaves)
	{
		//Serial.println("looping");
		ICTR_t* slaveCTR = slave->GetCTR();

		if (slaveCTR)
		{
			ICTR_T_WRITE(*slaveCTR, slave, Message::BuildInitRequestMessage(slave->GetID()));
			ICTR_T_WRITE(*slaveCTR,,Message::BuildReInitSlaveMessage());
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
	if (!fShouldSendLinkInfo || !espNowCore || !masterCTR.index())
		return;

	ESP_LOGI("LINK", "HANDLING LINK INFO");

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

	msgBuffer[bufIndex] = Message::Frame::End;

	Message msg(msgBuffer, msgSize);

	std::visit([&msg](auto&& ctr) {

			using T = std::decay_t<decltype(ctr)>;

			if constexpr (!std::is_same_v<T, std::monostate>)
				ctr.Write(msg);

			}, masterCTR);

	fShouldSendLinkInfo = false;
}
