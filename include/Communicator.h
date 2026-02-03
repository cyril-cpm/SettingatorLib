#pragma once

#include "Message.h"
#include <sys/_stdint.h>
#include <queue>
#include <variant>
#include "Message.h"
#include <mutex>

extern std::mutex newSlavesCTRMutex;
extern std::mutex reconnectedSlavesCTRMutex;

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
	- Write Buffer to communicator
	*/
	int Write(this auto&& self, Message& buf) {
		return self.WriteImpl(buf);
	}

	int Write(this auto&& self, Message&& buf) {
		return self.WriteImpl(buf);
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

	uint16_t	GetLinkInfoSize() const;

	void		WriteLinkInfoToBuffer(uint8_t* buffer) const;

	protected:
	
	ICTR() = default;

	void _receive(Message* msg);

	std::queue<Message*> fReceivedMessage;

};


