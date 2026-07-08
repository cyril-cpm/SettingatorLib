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
		// ConfigLinkDirectNotif = 0x15,
		// ConfigLinkDirectSettingUpdate = 0x16,
		RemoveDirectNotifConfig = 0x17,
		RemoveDirectSettingUpdateConfig = 0x18,
		BroadcastedPing = 0x19,
		MultiSettingUpdate = 0x1B,

		/// Bridge
		BridgeBase = 0x50,
		// LinkInitWithSSD = 0x54,
		// LinkConfigDirectNotif = 0x55,
		// LinkConfigDirectSettingUpdate = 0x56,
		// LinkRemoveDirectNotifConfig = 0x57,
		// LinkRemoveDirectSettingUpdateConfig = 0x58,
		LinkStartInitBroadcastedSlave = 0x59,
		LinkStopInitBroadcastedSlave = 0x5A,
		BridgeReinitSlaves = 0x5B,
		SlaveIDRequest = 0x5C,
		// LinkPing = 0x5D,
		// LinkPong = 0x5E,
		LinkInfo = 0x5F
	};

	enum Frame
	{
		Start = 0xFF,
		End = 0x00
	};

};
