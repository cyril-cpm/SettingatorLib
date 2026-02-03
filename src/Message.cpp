#include "Message.h"
#include "MiscDef.h"
#include <vector>

#if defined(ARDUINO)
#include <Arduino.h>

#elif defined(ESP_PLATFORM)
#include <stdlib.h>
#include <esp_types.h>
#include <cstring>

#endif

Message Message::BuildInitRequestMessage(uint8_t slaveID)
{
	return Message({
			Message::Frame::Start,
			0x00,
			0x07,
			slaveID,
			Message::Type::InitRequest,
			0,
			Message::Frame::End
		});
}

Message Message::BuildSlaveIDRequestMessage()
{
	return Message({
		Message::Frame::Start,
		0x00,
		0x06,
		0,
		Message::Type::SlaveIDRequest,
		Message::Frame::End
	});
}

Message Message::BuildReInitSlaveMessage()
{
	return Message({
		Message::Frame::Start,
		0x00,
		0x06,
		0,
		Message::Type::BridgeReinitSlaves,
		Message::Frame::End
	});
}

uint16_t Message::GetLength()
{
	return fBuffer.size();
}

uint8_t* Message::GetBufPtr()
{
	if (fBuffer.size())
		return fBuffer.data();
	else
		return nullptr;
}

void Message::_initMessageFromBuffer()
{
	if (fBuffer.size() < 4)
		return;

	fType = (Type)fBuffer[4];
	fSlaveID = fBuffer[3];
}

uint16_t Message::ExtractSettingUpdate(uint8_t &ref, uint8_t &newValueLen, uint8_t **newValue, uint16_t settingIndex)
{
	//Serial.println("Extracting Setting Update message");
	ref = 0;
	newValueLen = 0;
	*newValue = nullptr;

	if (fType == Message::Type::SettingUpdate)
	{
		ref = fBuffer[settingIndex];
		newValueLen = fBuffer[settingIndex + 1];
		*newValue = &fBuffer[settingIndex + 2];
		return settingIndex + 2 + newValueLen;

	}
	else
	{
	}
	//Serial.println("Done");
	return (fLength - 1);
}

char* Message::ExtractSSD()
{
	return (char*)(fBuffer.data() + 5);
}

uint8_t Message::operator[](uint16_t index)
{
	if (index >= fLength)
		index = fLength - 1;

	return fBuffer[index];
}
