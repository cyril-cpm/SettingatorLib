#pragma once

#include "Core.h"

class ESPNowCore
{
    public:
    ESPNowCore();

    void     Write(std::initializer_list<uint8_t>message,
			const std::array<uint8_t, 6>& dstMac) {
		esp_now_send(dstMac.data(), message.begin(), message.size());
	}

	void	Write(const std::array<uint8_t, 6>& dstMac) {
		esp_now_send(dstMac.data(), messageBuffer.data(), messageBuffer.len());
	}

    void    Update();
    void    AddPeer(const std::array<uint8_t, 6>& peerMac);
    void    BroadcastPing();
    const std::array<uint8_t, 6>&    GetMac() const {return fMac; }
    void    CreateLinkInfoTimer();
    void    HandleLinkInfo();
    void    shouldsendlinkinfo(bool should = true);

    static void         receiveCallback(const esp_now_recv_info* info, const uint8_t* data, int len);

    private:

	std::array<uint8_t, 6>	fMac;
    TimerHandle_t			fLinkInfoTimer = nullptr;
    uint32_t				fEspNowVersion = 0;
};

ESPNowCore& GetInstance()
{
	static ESPNowCore instance;
	return instance;
}
