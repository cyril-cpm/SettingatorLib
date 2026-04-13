#pragma once

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

};
