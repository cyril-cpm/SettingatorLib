#include "ESPNowCommunicator.h"
#include "Communicator.h"
#include "MiscDef.h"
#include "Message.h"
#include "Slave.h"
#include <cstdint>
#include <esp_now.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_mac.h>
#include <mutex>

#if defined(ARDUINO)

#elif defined(ESP_PLATFORM)
#include <cstring>
#include <esp_log.h>
#endif

ESPNowCore* espNowCore = nullptr;

std::mutex espNowMsgListMutex;

std::vector<std::pair<uint8_t*, std::queue<espNowMsg*>>> espNowMsgList;
std::vector<ESPNowCTR*> ESPNowCTR::fCTRList;

//std::queue<espNowMsg*> espNowMsgList;

static bool compareMac(const uint8_t* mac1, const uint8_t* mac2) {
	return memcmp(mac1, mac2, 6) == 0;
}

static std::queue<espNowMsg*>* findQueueForMac(const uint8_t* macToFind) {
	for (auto& pair : espNowMsgList) {
		if (compareMac(pair.first, macToFind)) {
			return &pair.second;
		}
	}
	return nullptr; // Si non trouvé
}

espNowMsg::espNowMsg(const uint8_t* inData, int inLen, uint32_t inTimestamp, int8_t inNoiseFloor, int8_t inRssi) 
		: len(inLen), timestamp(inTimestamp), noiseFloor(inNoiseFloor), rssi(inRssi)
{
	data = (uint8_t*)malloc(sizeof(uint8_t) * len);
	if (data)
		memcpy(data, inData, len * sizeof(uint8_t));
}

espNowMsg::~espNowMsg()
{
	delete data;
}


static bool isBroadcastMac(uint8_t* dstMac)
{
	bool isBroadcastMac = true;

	if (dstMac)
	{
		for (auto i = 0; i < 6; i++)
		{
			if (dstMac[i] != 0xFF)
				isBroadcastMac = false;
		}
	}
	else
		isBroadcastMac = false;

	return isBroadcastMac;
}

#if defined(ARDUINO)
static void receiveCallback(const uint8_t* mac, const uint8_t* data, int len)
{
	if (mac)
	{
		if (initEspNowBroadcasted && len == 1 && data && *data == 0x42)
		{
			ICTR_t newCTR = ESPNowCTR::CreateInstanceWithMac(mac);

			newSlavesCTR.push(newCTR);
		}
		else
		{
			if (!masterCTR) //A changer
				masterCTR = ESPNowCTR::CreateInstanceWithMac(mac);

			espNowMsgListMutex.lock();

			auto list = findQueueForMac(mac);

			if (list)
				list->push(new espNowMsg(data, len));

			espNowMsgListMutex.unlock();
		}
	}
}

#elif defined(ESP_PLATFORM)
void ESPNowCore::receiveCallback(const esp_now_recv_info* info, const uint8_t* data, int len)
{
	 if (info)
	{
		if (initEspNowBroadcasted && len == 1 && data && *data == 0x42 && isBroadcastMac(info->des_addr))
		{
			// ICTR_t newCTR = ESPNowCTR::FindCTRForMac(info->src_addr);
			//
			//
			// if (!newCTR.index())
			// 	newCTR = ESPNowCTR::CreateInstanceWithMac(info->src_addr, true);
			//
			// newSlavesCTR.push(newCTR);

			ESPNowCTR*	existingCTR = ESPNowCTR::GetCTRForMac(info->src_addr);
			if (existingCTR)
				newSubSlavesCTR.push(existingCTR);
			else
				newSlavesCTR.emplace(ESPNowCTR::CreateInstanceWithMac(info->src_addr, true));
		}
		else
		{
			if (masterCTR.index())
				masterCTR = ESPNowCTR::CreateInstanceWithMac(info->src_addr);

			espNowMsgListMutex.lock();
			
			auto list = findQueueForMac(info->src_addr);

			if (list)
			{
				if (info->rx_ctrl)
					list->push(new espNowMsg(data, len, info->rx_ctrl->timestamp, info->rx_ctrl->noise_floor, info->rx_ctrl->rssi));
				else
					list->push(new espNowMsg(data, len));
			}

			espNowMsgListMutex.unlock();
		}
	}
}

#endif

ESPNowCore* ESPNowCore::CreateInstance()
{
	if (!espNowCore)
		espNowCore = new ESPNowCore();

	return espNowCore;
}

ESPNowCore::ESPNowCore()
{
	//NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());  // Efface la mémoire si nécessaire
		ret = nvs_flash_init();  // Réinitialise NVS
	}

	//WIFI
	esp_err_t err = esp_event_loop_create_default();
	if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
		ESP_ERROR_CHECK(err);

	wifi_init_config_t wifiCfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifiCfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));

	//NOW
	ESP_ERROR_CHECK(esp_now_init());
	ESP_ERROR_CHECK(esp_now_register_recv_cb(receiveCallback));
	
	ESP_ERROR_CHECK(esp_read_mac(fMac, ESP_MAC_WIFI_STA));	// Récupération de l'adresse MAC Wi-Fi (STA)

	ESP_LOGI("MAC", "Adresse MAC (STA) : %02X:%02X:%02X:%02X:%02X:%02X",
			 fMac[0], fMac[1], fMac[2], fMac[3], fMac[4], fMac[5]);

	ESP_ERROR_CHECK(esp_now_get_version(&fEspNowVersion));
}

int ESPNowCore::Write(Message&& buf, uint8_t* dstMac)
{
	//ESP_LOGI("ESPNowCTR", "Esp now write");

	//DEBUG_PRINT_LN(buf.GetLength())
	//if (buf.GetLength() > 250)
	//	 DEBUG_PRINT_LN("Alert ! message to big for ESPNow !")

	//ESP_LOG_BUFFER_HEX("ESPNowCore", dstMac, 6);
	esp_now_send(dstMac, buf.GetBufPtr(), buf.GetLength());
	return 0;
}

void ESPNowCore::AddPeer(uint8_t* peerMac)
{
	esp_now_peer_info peerInfo;

	if (esp_now_get_peer(peerMac, &peerInfo) == ESP_ERR_ESPNOW_NOT_FOUND)
	{
		peerInfo = {};
		memcpy(peerInfo.peer_addr, peerMac, 6);
		peerInfo.channel = 1;
		peerInfo.encrypt = false;

		esp_now_add_peer(&peerInfo);
	}
}

void ESPNowCore::BroadcastPing()
{
	uint8_t dstMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t data = 0x42;

	AddPeer(dstMac);

	ESP_ERROR_CHECK(esp_now_send(dstMac, &data, sizeof(data)));
}

const uint8_t*	  ESPNowCore::GetMac() const
{
	return fMac;
}

ESPNowCTR ESPNowCTR::CreateInstanceWithMac(const uint8_t* mac, const bool createTimer)
{
	return ESPNowCTR(mac, createTimer);
}

void ESPNowCTR::SendPing()
{
	uint8_t buffer[6] = {
		Message::Frame::Start,
		0x00,
		0x06,
		0,
		Message::Type::EspNowPing,
		Message::Frame::End
	};
	
	Message msg(buffer, 6);

	Write(msg);

	ESP_LOGI("ESPNow", "SendPing");

	if (fPingTimer)
		xTimerChangePeriod(fPingTimer, pdMS_TO_TICKS(2000), 0);
}

void ESPNowCTR::SendPong()
{
	uint8_t buffer[12] = {
		Message::Frame::Start,
		0x00,
		0x0C,
		0,
		Message::Type::EspNowPong,
		(uint8_t)fLastMsgRssi,
		(uint8_t)fLastMsgNoiseFloor,
		0,
		0,
		0,
		0,
		Message::Frame::End
	};

	uint32_t deltaMs = pdTICKS_TO_MS(xTaskGetTickCount()) - fLastMsgTimestamp;

	memcpy(buffer + 7, (uint8_t*)&deltaMs, 4);

	Message msg(buffer, 12);

	Write(msg);

	ESP_LOGI("ESPNow", "SendPong");
}

void pingTimerCallback(TimerHandle_t timer)
{
	ESPNowCTR* ctr = (ESPNowCTR*)pvTimerGetTimerID(timer);

	ctr->ShouldSendPing();
}

ESPNowCTR::ESPNowCTR(const uint8_t* peerMac, const bool createTimer)
{

	fCore = ESPNowCore::CreateInstance();

	//fCore->CreateLinkInfoTimer();

	if (peerMac != nullptr)
	{
		ESP_LOGI("ESPNowCTR", "mac not null");
		fMac = (uint8_t*)malloc(6 * sizeof(uint8_t));
		memcpy(fMac, peerMac, 6 * sizeof(uint8_t));
		
		fCore->AddPeer(fMac);

		std::queue<espNowMsg*> msgList;

		espNowMsgList.emplace_back(fMac, msgList); 
	}

	if (createTimer)
	{
		fPingTimer = xTimerCreate(
			"pingTimer",
			pdMS_TO_TICKS(5000),
			pdTRUE,
			(void*)this,
			pingTimerCallback
		);

		 xTimerStart(fPingTimer, 0);
	}

	fCTRList.push_back(this);

	ESP_LOGI("ESPNowCTR", "end init");
}

void ESPNowCTR::ShouldSendPing(bool should)
{
	fShouldSendPing = should;
}

void ESPNowCTR::_bufferizeMessage(espNowMsg* msg)
{
	if (fMessageBuffer == nullptr)
	{
		fMessageBuffer = (uint8_t*)malloc(msg->len * sizeof(uint8_t));
		fMessageBufferSize = msg->len;
		memcpy(fMessageBuffer, msg->data, msg->len);
	}
	else
	{
		uint8_t* tmpBuf = fMessageBuffer;

		fMessageBuffer = (uint8_t*)malloc((fMessageBufferSize + msg->len) * sizeof(uint8_t));
		memcpy(fMessageBuffer, tmpBuf, fMessageBufferSize);
		memcpy(fMessageBuffer + fMessageBufferSize, msg->data, msg->len);
		fMessageBufferSize += msg->len;
		delete tmpBuf;
	}

	uint16_t msgSize = 0;
			
	if (fMessageBufferSize >= 3)
		msgSize = (fMessageBuffer[1] << 8) + fMessageBuffer[2];

	if (msgSize <= fMessageBufferSize && fMessageBuffer[msgSize-1] == Message::Frame::End)
	{
		_receive(new Message(fMessageBuffer, fMessageBufferSize));
		fMessageBufferSize = 0;
		delete fMessageBuffer;
	}
}

void ESPNowCTR::UpdateImpl()
{
	if (fShouldSendPing)
	{
		SendPing();
		fShouldSendPing = false;
	}

	if (espNowMsgListMutex.try_lock())
	{
		auto list = findQueueForMac(fMac);


		while (list && list->size())
		{
			auto msg = list->front();

			if (list->size() == 1)
			{
				fLastMsgTimestamp = pdTICKS_TO_MS(xTaskGetTickCount());
				fLastMsgRssi = msg->rssi;
				fLastMsgNoiseFloor = msg->noiseFloor;

				if (fPingTimer)
					xTimerChangePeriod(fPingTimer, pdMS_TO_TICKS(5000), 0);
			}

			uint16_t msgSize = 0;
			
			if (msg->len >= 3)
				msgSize = (msg->data[1] << 8) + msg->data[2];

			if (msgSize > msg->len || msg->data[msgSize-1] != Message::Frame::End)
				_bufferizeMessage(msg);
			else
			{
				Message* newMessage = new Message(msg->data, msg->len);
 
				if (newMessage)
				{
					if (newMessage->GetType() == Message::Type::EspNowPong)
					{
						fPeerLastMsgRssi = newMessage->GetBufPtr()[5];
						fPeerLastMsgNoiseFloor = newMessage->GetBufPtr()[6];
						memcpy(&fPeerLastMsgDeltastamp, newMessage->GetBufPtr() + 7, 4);
						ESP_LOGI("ESPNow", "ReceivePong");
					}
					else
						SendPong();						   
				}
				_receive(newMessage);
			}

			delete msg;
			list->pop();
		}
		espNowMsgListMutex.unlock();
	}
}

int ESPNowCTR::WriteImpl(Message& buf)
{
	DEBUG_PRINT_LN(buf.GetLength())
	auto bufLength = buf.GetLength();

	for (int i = 0; i < bufLength; i += 250)
		esp_now_send(fMac, buf.GetBufPtr() + i, (bufLength - i) > 250 ? 250 : (bufLength - i));

	return 0;
}

espNowDirectNotif::espNowDirectNotif(const uint8_t* inMac, uint8_t inNotifByte, uint8_t inDstSlaveID) : notifByte(inNotifByte), dstSlaveID(inDstSlaveID)
{
	mac = (uint8_t*)malloc(6);
	memcpy(mac, inMac, 6);
}

espNowDirectNotif::~espNowDirectNotif()
{
	delete mac;
}

espNowDirectSettingUpdate::espNowDirectSettingUpdate(const uint8_t* inMac, uint8_t inSettingRef, uint8_t inDstSlaveID, uint8_t inValueLen) : settingRef(inSettingRef), dstSlaveID(inDstSlaveID), valueLen(inValueLen)
{
	mac = (uint8_t*)malloc(6);
	memcpy(mac, inMac, 6);
}

espNowDirectSettingUpdate::~espNowDirectSettingUpdate()
{
	delete mac;
}

void ESPNowCTR::ConfigEspNowDirectNotif(uint8_t* mac, uint8_t notifByte, uint8_t dstSlaveID)
{
	fCore->AddPeer(mac);

	fDirectNotif.push_back(new espNowDirectNotif(mac, notifByte, dstSlaveID));
}

void ESPNowCTR::ConfigEspNowDirectSettingUpdate(uint8_t* mac, uint8_t settingRef, uint8_t settingValueLen, uint8_t dstSlaveID)
{
	fCore->AddPeer(mac);

	fDirectSettingUpdate.push_back(new espNowDirectSettingUpdate(mac, settingRef, dstSlaveID, settingValueLen));
}

void ESPNowCTR::RemoveDirectNotifConfig(uint8_t dstSlaveID, uint8_t notifByte)
{
	for (auto i = fDirectNotif.begin(); i != fDirectNotif.end(); i++)
	{
		if ((*i)->dstSlaveID == dstSlaveID && (*i)->notifByte == notifByte)
		{
			fDirectNotif.erase(i);
			break;
		}
	}
}

void ESPNowCTR::RemoveDirectSettingUpdateConfig(uint8_t dstSlaveID, uint8_t settingRef)
{
	for (auto i = fDirectSettingUpdate.begin(); i != fDirectSettingUpdate.end(); i++)
	{
		if ((*i)->dstSlaveID == dstSlaveID && (*i)->settingRef == settingRef)
		{
			fDirectSettingUpdate.erase(i);
			break;
		}
	}
}

void ESPNowCTR::SendDirectNotif(uint8_t notifByte)
{
	for (auto i = fDirectNotif.begin(); i != fDirectNotif.end(); i++)
	{
		if ((*i)->notifByte == notifByte)
		{
			fCore->Write(Message({
						Message::Frame::Start,
						0,
						7,
						(*i)->dstSlaveID,
						Message::Type::Notif,
						notifByte,
						Message::Frame::End
					}), (*i)->mac);
		}
	}
}

void ESPNowCTR::SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen)
{
	DEBUG_PRINT_LN("Call to SendDirectSettingUpdate")
	for (auto i = fDirectSettingUpdate.begin(); i != fDirectSettingUpdate.end(); i++)
	{
		if ((*i)->settingRef == settingRef)
		{
			if (valueLen > (*i)->valueLen)
				valueLen = (*i)->valueLen;

			uint16_t msgSize = 8 + valueLen;

			std::vector<uint8_t> msgBuffer(msgSize);

			msgBuffer[0] = Message::Frame::Start;
			msgBuffer[1] = msgSize >> 8;
			msgBuffer[2] = msgSize;
			msgBuffer[3] = (*i)->dstSlaveID;
			msgBuffer[4] = Message::Type::SettingUpdate;
			msgBuffer[5] = (*i)->settingRef;
			msgBuffer[6] = valueLen;
			
			if (valueLen)
				memcpy(&(msgBuffer[7]), value, valueLen);

			msgBuffer[msgSize-1] = Message::Frame::End;
			
			DEBUG_PRINT_VALUE_BUF_LN("Message sent", msgBuffer.data(), msgSize)

			fCore->Write(Message(std::move(msgBuffer)), (*i)->mac);
		}
	}
}

ESPNowCTR* ESPNowCTR::GetCTRForMac(const uint8_t* mac)
{
	for (ESPNowCTR* ctr : fCTRList)
	{
		if (ctr && compareMac(ctr->fMac, mac))
			return ctr;
	}
	return nullptr;
}

uint16_t ESPNowCTR::GetLinkInfoSize() const
{
	return 19;
}

void ESPNowCTR::WriteLinkInfoToBuffer(uint8_t* buffer) const
{
	buffer[0] = ICTR::LinkType::ESP_NOW;

	memcpy(buffer + 1, fMac, 6);

	buffer[7] = fLastMsgRssi;
	buffer[8] = fLastMsgNoiseFloor;

	uint32_t deltaMs = pdTICKS_TO_MS(xTaskGetTickCount()) - fLastMsgTimestamp;

	memcpy(buffer + 9, (uint8_t*)&deltaMs, 4);

	buffer[13] = fPeerLastMsgRssi;
	buffer[14] = fPeerLastMsgNoiseFloor;

	memcpy(buffer + 15, (uint8_t*)&(fPeerLastMsgDeltastamp), 4);

	return;
}
