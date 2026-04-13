#pragma once

#include "Message.h"
#include <initializer_list>
#include <sys/_stdint.h>
#include <queue>
#include <type_traits>
#include <variant>
#include "MiscDef.h"
#include "UARTCore.h"
#include "ESPNowCore.h"
#include <mutex>

class ICTR
{
	public:

	enum LinkType
	{
		ESP_NOW = 0x00,
		UART = 0x01,
		UNKNOWN = 0xFF
	};

	/*
	- return true if there is bytes available to read
	*/
	bool Available(this auto&& self) {
		self.Update();
		return self.fReceivedMessage.size();
	}

	/*
	- Read a message if avaible or return empty Message
	 */
	Message* Read();

	/*
	- Flush message after having executed
	*/
	void	Flush();

	/*
	- Update internal Buffer
	*/
	void Update(this auto&& self) {
		self.UpdateImpl();
	}

	uint8_t GetBoxSize() const;

	void ConfigEspNowDirectNotif(uint8_t* mac, uint8_t notifByte, uint8_t dstSlaveID);

	void ConfigEspNowDirectSettingUpdate(uint8_t* mac, uint8_t settingRef, uint8_t settingValueLen, uint8_t dstSlaveID);

	void SendDirectNotif(uint8_t notifByte);

	void SendDirectSettingUpdate(uint8_t settingRef, uint8_t* value, uint8_t valueLen);

	void RemoveDirectNotifConfig(uint8_t dstSlaveID, uint8_t notifByte);

	void RemoveDirectSettingUpdateConfig(uint8_t dstSlaveID, uint8_t settingRef);

	uint16_t	GetLinkInfoSize(this auto&& self) {
		return self.GetLinkInfoSizeImpl();
	}

	void		WriteLinkInfoToBuffer(this auto&& self, uint16_t index) {
		self.WriteLinkInfoToBufferImpl(index);
	}

	protected:
	
	ICTR() = default;

	void _receive(Message&& msg);

	std::queue<Message> fReceivedMessage;
};


