#pragma once

#include "Message.h"
#include <initializer_list>
#include <sys/_stdint.h>
#include <queue>
#include <type_traits>
#include <variant>
#include "Message.h"
#include "MiscDef.h"
#include "UARTCore.h"
#include "ESPNowCore.h"
#include <mutex>

extern std::mutex newSlavesCTRMutex;
extern std::mutex reconnectedSlavesMutex;

using CORE_t = std::variant<std::monostate, UARTCore, ESPNowCore>;

class ICTR
{
	public:

	enum LinkType
	{
		ESP_NOW = 0x00,
		UART = 0x01,
		UNKNOWN = 0xFF
	};

	ICTR(CORE_t& core) : fCore(core) {}

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
	int Write(std::initializer_list<uint8_t> message) {
		// static const char* tag("CTR");
		// LOG("Write:");
		// LOG_BUFFER_HEX(buf.GetBufPtr(), buf.GetLength())
		return std::visit([&message](auto&& core) -> int {

				using T = std::decay_t<decltype(core)>;

				if constexpr (!std::is_same_v<T, std::monostate>)
					return core.Write(message);
				return 0;
			}, fCore);
	}

	int Write() {
		// static const char* tag("CTR");
		// LOG("Write:");
		// LOG_BUFFER_HEX(buf.GetBufPtr(), buf.GetLength())
		return std::visit([](auto&& core) -> int {

				using T = std::decay_t<decltype(core)>;

				if constexpr (!std::is_same_v<T, std::monostate>)
					return core.Write();
				return 0;
			}, fCore);
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
	
	ICTR() = delete;

	void _receive(Message&& msg);

	std::queue<Message> fReceivedMessage;

	CORE_t&		fCore;

};


