#ifndef _MESSAGE_
#define _MESSAGE_

#include <initializer_list>
#include <vector>
#include <sys/_stdint.h>

class Message
{
public:
	enum Type
	{
		Uninitialised,

		/// Settingator
		InitRequest = 0x12,
		SettingUpdate = 0x11,
		SettingInit = 0x13,
		Notif = 0x14,
		ConfigEspNowDirectNotif = 0x15,
		ConfigEspNowDirectSettingUpdate = 0x16,
		RemoveDirectNotifConfig = 0x17,
		RemoveDirectSettingUpdateConfig = 0x18,
		BroadcastedPing = 0x19,
		MultiSettingUpdate = 0x1B,

		/// Bridge
		BridgeBase = 0x50,
		EspNowInitWithSSD = 0x54,
		EspNowConfigDirectNotif = 0x55,
		EspNowConfigDirectSettingUpdate = 0x56,
		EspNowRemoveDirectNotifConfig = 0x57,
		EspNowRemoveDirectSettingUpdateConfig = 0x58,
		EspNowStartInitBroadcastedSlave = 0x59,
		EspNowStopInitBroadcastedSlave = 0x5A,
		BridgeReinitSlaves = 0x5B,
		SlaveIDRequest = 0x5C,
		EspNowPing = 0x5D,
		EspNowPong = 0x5E,
		LinkInfo = 0x5F
	};

	enum Frame
	{
		Start = 0xFF,
		End = 0x00
	};

	static Message BuildInitRequestMessage(uint8_t slaveID);
	static Message BuildSlaveIDRequestMessage();
	static Message BuildReInitSlaveMessage();

	Message() = delete;

	Message(std::initializer_list<uint8_t> buffer) : fBuffer(buffer) {}; // inplace init
	Message(uint8_t* buffer, uint16_t len) : fBuffer(buffer, buffer + len) {}; // copy buf
	Message(std::vector<uint8_t>&& buffer) : fBuffer(std::move(buffer)) {}; // move buf

	/*
	- Return length of buffer
	*/
	uint16_t GetLength();

	/*
	- Return fBuffer ptr
	*/
	uint8_t*   GetBufPtr();

	/*
	- Return message type
	*/
   Type GetType() const { return fType; }

   uint8_t GetSlaveID() const { return fSlaveID; }

	/*
	- Get Setting Update Message
	- Get 0 or null if wrong message type
	*/
   uint16_t ExtractSettingUpdate(uint8_t &ref, uint8_t &newValueLen, uint8_t **newValue, uint16_t settingIndex = 5);

   char*	ExtractSSD();

   uint8_t operator[](uint16_t index);

private:
	std::vector<uint8_t>   fBuffer;
	Type	fType = Uninitialised;
	uint8_t fSlaveID = 0;
	uint16_t fLength;
};

#endif
