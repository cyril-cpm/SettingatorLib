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




void pingTimerCallback(TimerHandle_t timer)
{
	ESPNowCTR* ctr = (ESPNowCTR*)pvTimerGetTimerID(timer);

	ctr->ShouldSendPing();
}

ESPNowCTR::ESPNowCTR(CORE_t core, const std::array<uint8_t, 6>& peerMac, const bool createTimer)
	:
		ICTR(core),
		fMac(peerMac)
{
	//ESPNowCore::GetInstance().CreateLinkInfoTimer();

	fCore.AddPeer(fMac);

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
