#pragma once

#include "Definitions.h"

#if CONFIG_STR_HAS_BRIDGE

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

#include "Slave.h"
#include "Communicator.h"


class CTRBridge
{
	public:

	static	CTRBridge&	GetInstance() {
		static CTRBridge instance;
		return instance;
	}

	void	test();
	void	begin();

	void			StartEspNowInitBroadcasted();
	void			StopEspNowInitBroadcasted();
	void			Update();
	void			HandleLinkInfo();
    void			ShouldSendLinkInfo(bool should = true);
	void			CreateLinkInfoTimer();

	private:

	void			_addEspNowSlaveWithSSD(char* SSD, uint8_t slaveID);
	void			_configDirectNotif(Message& msg);
	void			_configDirectSettingUpdate(Message& msg);
	void			_removeDirectMessageConfig(Message& msg, uint8_t messageType);
	void			_reinitSlaves();
	void			_treatSettingInit(Message& msg, Slave& slave);

	bool			fShouldSendLinkInfo = false;
	TimerHandle_t	fLinkInfoTimer = nullptr;
};

#endif
