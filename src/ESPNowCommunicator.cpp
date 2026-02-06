#include "ESPNowCommunicator.h"
#include "Communicator.h"
#include "MiscDef.h"
#include "Message.h"
#include "Slave.h"
#include <array>
#include <cstdint>
#include <esp_now.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <esp_mac.h>
#include <mutex>
#include <cstring>
#include <esp_log.h>
#include <queue>
#include <vector>

static const char* tag("ESPNOWCTR");

std::mutex espNowMsgListMutex;

std::vector<std::pair<std::array<uint8_t, 6>, std::queue<espNowMsg>>> espNowMsgList;
std::vector<ESPNowCTR*> ESPNowCTR::fCTRList;

//std::queue<espNowMsg*> espNowMsgList;

bool compareMac(const uint8_t* mac1, const uint8_t* mac2) {
	return memcmp(mac1, mac2, 6) == 0;
}

static std::queue<espNowMsg>* findQueueForMac(const std::array<uint8_t, 6>& macToFind) {
	// LOG("Searching for mac:");
	// LOG_BUFFER_HEX(macToFind.data(), 6);

	for (auto& pair : espNowMsgList)
	{
		// LOG("CHECKING :");
		// LOG_BUFFER_HEX(pair.first.data(), 6);
	
		if (pair.first == macToFind) 
		{
			// LOG("FOUND");
			return &pair.second;
		}
	}
	return nullptr; // Si non trouvé
}

espNowMsg::espNowMsg(const uint8_t* inData, int inLen, uint32_t inTimestamp, int8_t inNoiseFloor, int8_t inRssi) 
		: data(inData, inData + inLen), len(inLen), timestamp(inTimestamp), noiseFloor(inNoiseFloor), rssi(inRssi)
{
}


static bool isBroadcastMac(std::array<uint8_t, 6>& dstMac)
{
	return (dstMac == std::to_array<uint8_t>({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}));
}

void ESPNowCore::receiveCallback(const esp_now_recv_info* info, const uint8_t* data, int len)
{
	LOG("data received");
	 if (info)
	{
		std::array<uint8_t, 6> des_addrArr;
		if (info->des_addr)
			std::copy(info->des_addr, info->des_addr + 6, des_addrArr.begin());

		std::array<uint8_t, 6> src_addrArr;
		if (info->src_addr)
			std::copy(info->src_addr, info->src_addr + 6, src_addrArr.begin());

		if (initEspNowBroadcasted && len == 1 && data && *data == 0x42 && isBroadcastMac(des_addrArr))
		{
			LOG("broadcast data");
			Slave* existingSlave = Slave::GetSlaveForMac(src_addrArr);

			if (existingSlave)
			{
				LOG("existingSlave");
				reconnectedSlavesMutex.lock();
				reconnectedSlaves.push(existingSlave);
				reconnectedSlavesMutex.unlock();
			}
			else
			{
				LOG("newSlave");
				newSlavesCTRMutex.lock();
				newSlavesCTR.push(ESPNowCTR::CreateInstanceWithMac(src_addrArr, true));
				LOG("Slave created with following MAC");
				LOG_BUFFER_HEX(src_addrArr.data(), 6);
				newSlavesCTRMutex.unlock();
			}
		}
		else
		{
			LOG("Message received");
			if (!masterCTR.index())
				masterCTR = ESPNowCTR::CreateInstanceWithMac(src_addrArr);

			espNowMsgListMutex.lock();
			
			auto list = findQueueForMac(src_addrArr);

			if (list)
			{
				LOG("msg queue found");
				LOG_BUFFER_HEX(data, len);
				if (info->rx_ctrl)
					list->push(espNowMsg(data, len, info->rx_ctrl->timestamp, info->rx_ctrl->noise_floor, info->rx_ctrl->rssi));
				else
					list->push(espNowMsg(data, len));
				LOG("queue size: %d", list->size());
			}

			espNowMsgListMutex.unlock();
		}
	}
}

ESPNowCore& ESPNowCore::GetInstance()
{
	static ESPNowCore instance;
	return instance;
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
	
	ESP_ERROR_CHECK(esp_read_mac(fMac.data(), ESP_MAC_WIFI_STA));	// Récupération de l'adresse MAC Wi-Fi (STA)

	ESP_ERROR_CHECK(esp_now_get_version(&fEspNowVersion));
}

int ESPNowCore::Write(Message&& buf, const std::array<uint8_t, 6>& dstMac)
{

	//if (buf.GetLength() > 250)

	esp_now_send(dstMac.data(), buf.GetBufPtr(), buf.GetLength());
	return 0;
}

void ESPNowCore::AddPeer(const std::array<uint8_t, 6>& peerMac)
{
	esp_now_peer_info peerInfo;

	if (esp_now_get_peer(peerMac.data(), &peerInfo) == ESP_ERR_ESPNOW_NOT_FOUND)
	{
		peerInfo = {};
		memcpy(peerInfo.peer_addr, peerMac.data(), 6);
		peerInfo.channel = 1;
		peerInfo.encrypt = false;

		esp_now_add_peer(&peerInfo);
	}
}

void ESPNowCore::BroadcastPing()
{
	std::array<uint8_t, 6> dstMac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t data = 0x42;

	AddPeer(dstMac);

	ESP_ERROR_CHECK(esp_now_send(dstMac.data(), &data, sizeof(data)));
}

const std::array<uint8_t, 6>& ESPNowCore::GetMac() const
{
	return fMac;
}

ESPNowCTR ESPNowCTR::CreateInstanceWithMac(const std::array<uint8_t, 6>& mac, const bool createTimer)
{
	return ESPNowCTR(mac, createTimer);
}

const std::array<uint8_t, 6>& ESPNowCTR::GetMac() const
{
	return fMac;
}

void ESPNowCTR::SendPing()
{
	Write(Message({
				Message::Frame::Start,
				0x00,
				0x06,
				0,
				Message::Type::EspNowPing,
				Message::Frame::End
			}));


	if (fPingTimer)
		xTimerChangePeriod(fPingTimer, pdMS_TO_TICKS(2000), 0);
}

void ESPNowCTR::SendPong()
{
	uint32_t deltaMs = pdTICKS_TO_MS(xTaskGetTickCount()) - fLastMsgTimestamp;

	Write(Message({
		Message::Frame::Start,
		0x00,
		0x0C,
		0,
		Message::Type::EspNowPong,
		(uint8_t)fLastMsgRssi,
		(uint8_t)fLastMsgNoiseFloor,
		(uint8_t)(deltaMs >> 24),
		(uint8_t)(deltaMs >> 16),
		(uint8_t)(deltaMs >> 8),
		(uint8_t)(deltaMs),
		Message::Frame::End
	}));

}

void pingTimerCallback(TimerHandle_t timer)
{
	ESPNowCTR* ctr = (ESPNowCTR*)pvTimerGetTimerID(timer);

	ctr->ShouldSendPing();
}

ESPNowCTR::ESPNowCTR(const std::array<uint8_t, 6>& peerMac, const bool createTimer)
	:
		fMac(peerMac)
{
	//ESPNowCore::GetInstance().CreateLinkInfoTimer();

	ESPNowCore::GetInstance().AddPeer(fMac);

	espNowMsgList.emplace_back(fMac, std::queue<espNowMsg>());

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

}

void ESPNowCTR::ShouldSendPing(bool should)
{
	fShouldSendPing = should;
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
		std::queue<espNowMsg>* msgList = findQueueForMac(fMac);
		while (msgList && msgList->size())
		{
			LOG("Message Availlable");
			espNowMsg msg = std::move(msgList->front());
			msgList->pop();

			if (!msgList->size())
			{
				fLastMsgTimestamp = pdTICKS_TO_MS(xTaskGetTickCount());
				fLastMsgRssi = msg.rssi;
				fLastMsgNoiseFloor = msg.noiseFloor;

				if (fPingTimer)
					xTimerChangePeriod(fPingTimer, pdMS_TO_TICKS(5000), 0);
			}


			Message newMessage = Message(std::move(msg.data));

			if (newMessage.GetType() == Message::Type::EspNowPong)
			{
				fPeerLastMsgRssi = newMessage.GetBufPtr()[5];
				fPeerLastMsgNoiseFloor = newMessage.GetBufPtr()[6];
				memcpy(&fPeerLastMsgDeltastamp, newMessage.GetBufPtr() + 7, 4);
			}
			else
				SendPong();

			LOG("Receive MSG");
			_receive(std::move(newMessage));
		}
		espNowMsgListMutex.unlock();
	}
}

int ESPNowCTR::WriteImpl(Message& buf)
{
	auto bufLength = buf.GetLength();

	for (int i = 0; i < bufLength; i += 250)
		esp_now_send(fMac.data(), buf.GetBufPtr() + i, (bufLength - i) > 250 ? 250 : (bufLength - i));

	return 0;
}

espNowDirectNotif::espNowDirectNotif(const std::array<uint8_t, 6>& inMac, uint8_t inNotifByte, uint8_t inDstSlaveID)
	:
		mac(inMac),
		notifByte(inNotifByte),
		dstSlaveID(inDstSlaveID)
{}

espNowDirectSettingUpdate::espNowDirectSettingUpdate(const std::array<uint8_t, 6>& inMac, uint8_t inSettingRef, uint8_t inDstSlaveID, uint8_t inValueLen)
	:
		mac(inMac),
		settingRef(inSettingRef),
		dstSlaveID(inDstSlaveID),
		valueLen(inValueLen)
{}

void ESPNowCTR::ConfigEspNowDirectNotif(const std::array<uint8_t, 6>& mac, uint8_t notifByte, uint8_t dstSlaveID)
{
	ESPNowCore::GetInstance().AddPeer(mac);

	fDirectNotif.push_back(espNowDirectNotif(mac, notifByte, dstSlaveID));
}

void ESPNowCTR::ConfigEspNowDirectSettingUpdate(const std::array<uint8_t, 6>& mac, uint8_t settingRef, uint8_t settingValueLen, uint8_t dstSlaveID)
{
	ESPNowCore::GetInstance().AddPeer(mac);

	fDirectSettingUpdate.push_back(espNowDirectSettingUpdate(mac, settingRef, dstSlaveID, settingValueLen));
}

void ESPNowCTR::RemoveDirectNotifConfig(uint8_t dstSlaveID, uint8_t notifByte)
{
	for (auto i = fDirectNotif.begin(); i != fDirectNotif.end(); i++)
	{
		if (i->dstSlaveID == dstSlaveID && i->notifByte == notifByte)
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
		if (i->dstSlaveID == dstSlaveID && i->settingRef == settingRef)
		{
			fDirectSettingUpdate.erase(i);
			break;
		}
	}
}

void ESPNowCTR::SendDirectNotif(uint8_t notifByte)
{
	for (auto& directNotif : fDirectNotif)
	{
		if (directNotif.notifByte == notifByte)
		{
			ESPNowCore::GetInstance().Write(Message({
						Message::Frame::Start,
						0,
						7,
						directNotif.dstSlaveID,
						Message::Type::Notif,
						notifByte,
						Message::Frame::End
					}), directNotif.mac);
		}
	}
}

void ESPNowCTR::SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen)
{
	for (auto& directSettingUpdate : fDirectSettingUpdate)
	{
		if (directSettingUpdate.settingRef == settingRef)
		{
			if (valueLen > directSettingUpdate.valueLen)
				valueLen = directSettingUpdate.valueLen;

			uint16_t msgSize = 8 + valueLen;

			std::vector<uint8_t> msgBuffer(msgSize);

			msgBuffer[0] = Message::Frame::Start;
			msgBuffer[1] = msgSize >> 8;
			msgBuffer[2] = msgSize;
			msgBuffer[3] = directSettingUpdate.dstSlaveID;
			msgBuffer[4] = Message::Type::SettingUpdate;
			msgBuffer[5] = directSettingUpdate.settingRef;
			msgBuffer[6] = valueLen;
			
			if (valueLen)
				memcpy(&(msgBuffer[7]), value, valueLen);

			msgBuffer[msgSize-1] = Message::Frame::End;
			

			ESPNowCore::GetInstance().Write(Message(std::move(msgBuffer)), directSettingUpdate.mac);
		}
	}
}

ESPNowCTR* ESPNowCTR::GetCTRForMac(const std::array<uint8_t, 6>& mac)
{
	for (ESPNowCTR* ctr : fCTRList)
	{
		if (ctr && mac == ctr->GetMac())
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

	memcpy(buffer + 1, fMac.data(), 6);

	buffer[7] = fLastMsgRssi;
	buffer[8] = fLastMsgNoiseFloor;

	uint32_t deltaMs = pdTICKS_TO_MS(xTaskGetTickCount()) - fLastMsgTimestamp;

	memcpy(buffer + 9, (uint8_t*)&deltaMs, 4);

	buffer[13] = fPeerLastMsgRssi;
	buffer[14] = fPeerLastMsgNoiseFloor;

	memcpy(buffer + 15, (uint8_t*)&(fPeerLastMsgDeltastamp), 4);

	return;
}
