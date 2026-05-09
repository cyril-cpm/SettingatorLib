#include "ESPNowCommunicator.h"
#include "Definitions.h"

#if STR_HAS_ESPNOW

#include "Communicator.h"
#include "ESPNowCore.h"
#include "MiscDef.h"
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
#include <optional>

std::array<std::optional<std::reference_wrapper<ESPNowCTR>>, NB_ESPNOW_CTR> espNowCtrArray;
uint8_t registeredEspNowCtr = 0;

static const char* tag("ESPNOWCTR");


//std::queue<espNowMsg*> espNowMsgList;

bool compareMac(const uint8_t* mac1, const uint8_t* mac2) {
	return memcmp(mac1, mac2, 6) == 0;
}

void pingTimerCallback(TimerHandle_t timer)
{
	ESPNowCTR* ctr = (ESPNowCTR*)pvTimerGetTimerID(timer);

	ctr->ShouldSendPing();
}

ESPNowCTR::ESPNowCTR(ESPNowCore& core, const std::array<uint8_t, 6>& peerMac, const bool createTimer)
	:
		fCore(core),
		fMac(peerMac)
{
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
	
}

void ESPNowCTR::ShouldSendPing(bool should)
{
	fShouldSendPing = should;
}

// espNowDirectNotif::espNowDirectNotif(const std::array<uint8_t, 6>& inMac, uint8_t inNotifByte, uint8_t inDstSlaveID)
// 	:
// 		mac(inMac),
// 		notifByte(inNotifByte),
// 		dstSlaveID(inDstSlaveID)
// {}
//
// espNowDirectSettingUpdate::espNowDirectSettingUpdate(const std::array<uint8_t, 6>& inMac, uint8_t inSettingRef, uint8_t inDstSlaveID, uint8_t inValueLen)
// 	:
// 		mac(inMac),
// 		settingRef(inSettingRef),
// 		dstSlaveID(inDstSlaveID),
// 		valueLen(inValueLen)
// {}
//
// void ESPNowCTR::ConfigEspNowDirectNotif(const std::array<uint8_t, 6>& mac, uint8_t notifByte, uint8_t dstSlaveID)
// {
// 	ESPNowCore::GetInstance().AddPeer(mac);
//
// 	fDirectNotif.push_back(espNowDirectNotif(mac, notifByte, dstSlaveID));
// }
//
// void ESPNowCTR::ConfigEspNowDirectSettingUpdate(const std::array<uint8_t, 6>& mac, uint8_t settingRef, uint8_t settingValueLen, uint8_t dstSlaveID)
// {
// 	ESPNowCore::GetInstance().AddPeer(mac);
//
// 	fDirectSettingUpdate.push_back(espNowDirectSettingUpdate(mac, settingRef, dstSlaveID, settingValueLen));
// }
//
// void ESPNowCTR::RemoveDirectNotifConfig(uint8_t dstSlaveID, uint8_t notifByte)
// {
// 	for (auto i = fDirectNotif.begin(); i != fDirectNotif.end(); i++)
// 	{
// 		if (i->dstSlaveID == dstSlaveID && i->notifByte == notifByte)
// 		{
// 			fDirectNotif.erase(i);
// 			break;
// 		}
// 	}
// }
//
// void ESPNowCTR::RemoveDirectSettingUpdateConfig(uint8_t dstSlaveID, uint8_t settingRef)
// {
// 	for (auto i = fDirectSettingUpdate.begin(); i != fDirectSettingUpdate.end(); i++)
// 	{
// 		if (i->dstSlaveID == dstSlaveID && i->settingRef == settingRef)
// 		{
// 			fDirectSettingUpdate.erase(i);
// 			break;
// 		}
// 	}
// }
//
// void ESPNowCTR::SendDirectNotif(uint8_t notifByte)
// {
// 	for (auto& directNotif : fDirectNotif)
// 	{
// 		if (directNotif.notifByte == notifByte)
// 		{
// 			fCore.Write({
// 						Message::Frame::Start,
// 						0,
// 						7,
// 						directNotif.dstSlaveID,
// 						Message::Type::Notif,
// 						notifByte,
// 						Message::Frame::End
// 					}, directNotif.mac);
// 		}
// 	}
// }
//
// void ESPNowCTR::SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen)
// {
// 	for (auto& directSettingUpdate : fDirectSettingUpdate)
// 	{
// 		if (directSettingUpdate.settingRef == settingRef)
// 		{
// 			if (valueLen > directSettingUpdate.valueLen)
// 				valueLen = directSettingUpdate.valueLen;
//
// 			uint16_t msgSize = 8 + valueLen;
//
// 			messageBuffer[0] = Message::Frame::Start;
// 			messageBuffer[1] = msgSize >> 8;
// 			messageBuffer[2] = msgSize;
// 			messageBuffer[3] = directSettingUpdate.dstSlaveID;
// 			messageBuffer[4] = Message::Type::SettingUpdate;
// 			messageBuffer[5] = directSettingUpdate.settingRef;
// 			messageBuffer[6] = valueLen;
//
// 			if (valueLen)
// 				memcpy(&(messageBuffer[7]), value, valueLen);
//
// 			messageBuffer[msgSize-1] = Message::Frame::End;
//
//
// 			fCore.Write(directSettingUpdate.mac);
// 		}
// 	}
// }
//
// ESPNowCTR* ESPNowCTR::GetCTRForMac(const std::array<uint8_t, 6>& mac)
// {
// 	for (ESPNowCTR* ctr : fCTRList)
// 	{
// 		if (ctr && mac == ctr->GetMac())
// 			return ctr;
// 	}
// 	return nullptr;
// }

uint16_t ESPNowCTR::GetLinkInfoSizeImpl() const
{
	return 19;
}

void ESPNowCTR::WriteLinkInfoToBufferImpl(uint16_t index) const
{
	messageBuffer[index] = ICTR::LinkType::ESP_NOW;

	memcpy(messageBuffer.data() + index + 1, fMac.data(), 6);

	messageBuffer[index + 7] = fLastMsgRssi;
	messageBuffer[index + 8] = fLastMsgNoiseFloor;

	uint32_t deltaMs = pdTICKS_TO_MS(xTaskGetTickCount()) - fLastMsgTimestamp;

	memcpy(messageBuffer.data() + index + 9, (uint8_t*)&deltaMs, 4);

	messageBuffer[index + 13] = fPeerLastMsgRssi;
	messageBuffer[index + 14] = fPeerLastMsgNoiseFloor;

	memcpy(messageBuffer.data() + index + 15, (uint8_t*)&(fPeerLastMsgDeltastamp), 4);

	return;
}

std::optional<std::reference_wrapper<ESPNowCTR>> GetESPNowCommunicatorByMac(const std::array<uint8_t, 6>& mac)
{
	for (auto& ctr : espNowCtrArray)
	{
		if (!ctr)
			break;

		if (mac == ctr->get().GetMac())
			return *ctr;
	}
	return std::nullopt;
}

#endif
