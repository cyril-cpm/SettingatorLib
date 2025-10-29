#pragma once

#include "Message.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <vector>

class ICTR;
class Slave;

class CTRBridge
{
	public:

	static	CTRBridge*   CreateInstance(ICTR* master);

	CTRBridge(ICTR* master);

	void	begin();

	void			StartEspNowInitBroadcasted();
	void			StopEspNowInitBroadcasted();
	void			Update();
	void			HandleLinkInfo();
    void			ShouldSendLinkInfo(bool should = true);
	void			CreateLinkInfoTimer();
	void			SetMaster(ICTR* master);

	private:

	void			_addEspNowSlaveWithSSD(char* SSD, uint8_t slaveID);
	void			_configDirectNotif(Message* msg);
	void			_configDirectSettingUpdate(Message* msg);
	void			_removeDirectMessageConfig(Message* msg, uint8_t messageType);
	void			_reinitSlaves();
	void			_treatSettingInit(Message* msg, Slave* slave);

	bool			fShouldSendLinkInfo = false;
	TimerHandle_t	fLinkInfoTimer = nullptr;
};

extern CTRBridge BRIDGE;
