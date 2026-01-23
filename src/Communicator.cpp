#include "Communicator.h"

#include "MiscDef.h"
#include "Message.h"
#include <variant>

bool initEspNowBroadcasted = false;

void ICTR::_receive(Message* msg)
{
    fReceivedMessage.push(msg);
}

void ICTR::ConfigEspNowDirectNotif(uint8_t* mac, uint8_t notifByte, uint8_t dstSlaveID)
{
    DEBUG_PRINT_LN("_configEspNowDirectNotif : Not an ESPNow Communicator")
}

void ICTR::ConfigEspNowDirectSettingUpdate(uint8_t* mac, uint8_t settingRef, uint8_t settingValueLen, uint8_t dstSlaveID)
{
    DEBUG_PRINT_LN("ConfigEspNowDirectSettingUpdate : Not an ESPNow Communicator")
}

void ICTR::SendDirectNotif(uint8_t notifByte)
{
    DEBUG_PRINT_LN("SendDirectNotif not implemented in CTR")
}

void ICTR::SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen)
{
    DEBUG_PRINT_LN("SendDirectSettingUpdate not implemented in CTR")
}

void ICTR::RemoveDirectNotifConfig(uint8_t dstSlaveID, uint8_t notifByte)
{
    DEBUG_PRINT_LN("RemoveDirectNotifConfig not implemented in CTR")
}

void ICTR::RemoveDirectSettingUpdateConfig(uint8_t dstSlaveID, uint8_t settingRef)
{
    DEBUG_PRINT_LN("RemoveDirectSettingUpdateConfig not implemented in CTR")
}

uint8_t ICTR::GetBoxSize() const
{
    return fReceivedMessage.size();
}


bool ICTR::Available(this auto&& self)
{
    self.Update();
    return self.fReceivedMessage.size();
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
    {
        delete fReceivedMessage.front();
        fReceivedMessage.pop();
    }
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

int ICTR::Write(this auto&& self, Message& buf)
{
	return self.WriteImpl(buf);
}

void ICTR::Update(this auto&& self)
{
	self.UpdateImpl();
}
