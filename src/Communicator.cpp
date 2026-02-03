#include "Communicator.h"

#include "MiscDef.h"
#include "Message.h"
#include "UARTCommunicator.h"
#include <variant>

std::mutex newSlavesCTRMutex;
std::mutex reconnectedSlavesMutex;

bool initEspNowBroadcasted = false;

void ICTR::_receive(Message* msg)
{
	fReceivedMessage.push(msg);
}

void ICTR::ConfigEspNowDirectNotif(uint8_t* mac, uint8_t notifByte, uint8_t dstSlaveID)
{
}

void ICTR::ConfigEspNowDirectSettingUpdate(uint8_t* mac, uint8_t settingRef, uint8_t settingValueLen, uint8_t dstSlaveID)
{
}

void ICTR::SendDirectNotif(uint8_t notifByte)
{
}

void ICTR::SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen)
{
}

void ICTR::RemoveDirectNotifConfig(uint8_t dstSlaveID, uint8_t notifByte)
{
}

void ICTR::RemoveDirectSettingUpdateConfig(uint8_t dstSlaveID, uint8_t settingRef)
{
}

uint8_t ICTR::GetBoxSize() const
{
	return fReceivedMessage.size();
}

Message* ICTR::Read()
{
	if (GetBoxSize())
		return fReceivedMessage.front();
	return nullptr;
}

void ICTR::Flush()
{
	if (!fReceivedMessage.empty())
		fReceivedMessage.pop();
}

uint16_t ICTR::GetLinkInfoSize() const
{
	return 1;
}

void ICTR::WriteLinkInfoToBuffer(uint8_t *buffer) const
{
	buffer[0] = ICTR::LinkType::UNKNOWN;
	return;
}
