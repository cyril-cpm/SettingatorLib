#include "ESPNowCore.h"
#include "MiscDef.h"

static bool isBroadcastMac(std::array<uint8_t, 6>& dstMac)
{
	return (dstMac == std::to_array<uint8_t>({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}));
}

void ESPNowCore::receiveCallback(const esp_now_recv_info* info, const uint8_t* data, int len)
{
	LOG("data received");
	 if (info)
	{
		std::array<uint8_t, 6> des_addrArr;
		if (info->des_addr)
			std::copy(info->des_addr, info->des_addr + 6, des_addrArr.begin());

		std::array<uint8_t, 6> src_addrArr;
		if (info->src_addr)
			std::copy(info->src_addr, info->src_addr + 6, src_addrArr.begin());

		if (initEspNowBroadcasted && len == 1 && data && *data == 0x42 && isBroadcastMac(des_addrArr))
		{
			LOG("broadcast data");
			Slave* existingSlave = Slave::GetSlaveForMac(src_addrArr);

			if (existingSlave)
			{
				LOG("existingSlave");
				reconnectedSlavesMutex.lock();
				reconnectedSlaves.push(existingSlave);
				reconnectedSlavesMutex.unlock();
			}
			else
			{
				LOG("newSlave");
				newSlavesCTRMutex.lock();
				newSlavesCTR.push(ESPNowCTR::CreateInstanceWithMac(src_addrArr, true));
				LOG("Slave created with following MAC");
				LOG_BUFFER_HEX(src_addrArr.data(), 6);
				newSlavesCTRMutex.unlock();
			}
		}
		else
		{
			LOG("Message received");
			if (!masterCTR.index())
				masterCTR = ESPNowCTR::CreateInstanceWithMac(src_addrArr);

			espNowMsgListMutex.lock();
			
			auto list = findQueueForMac(src_addrArr);

			if (list)
			{
				LOG("msg queue found");
				LOG_BUFFER_HEX(data, len);
				if (info->rx_ctrl)
					list->push(espNowMsg(data, len, info->rx_ctrl->timestamp, info->rx_ctrl->noise_floor, info->rx_ctrl->rssi));
				else
					list->push(espNowMsg(data, len));
				LOG("queue size: %d", list->size());
			}

			espNowMsgListMutex.unlock();
		}
	}
}

ESPNowCore& ESPNowCore::GetInstance()
{
	static ESPNowCore instance;
	return instance;
}

ESPNowCore::ESPNowCore()
{
	//NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());  // Efface la mémoire si nécessaire
		ret = nvs_flash_init();  // Réinitialise NVS
	}

	//WIFI
	esp_err_t err = esp_event_loop_create_default();
	if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
		ESP_ERROR_CHECK(err);

	wifi_init_config_t wifiCfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifiCfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));

	//NOW
	ESP_ERROR_CHECK(esp_now_init());
	ESP_ERROR_CHECK(esp_now_register_recv_cb(receiveCallback));
	
	ESP_ERROR_CHECK(esp_read_mac(fMac.data(), ESP_MAC_WIFI_STA));	// Récupération de l'adresse MAC Wi-Fi (STA)

	ESP_ERROR_CHECK(esp_now_get_version(&fEspNowVersion));
}

int ESPNowCore::Write(Message&& buf, const std::array<uint8_t, 6>& dstMac)
{

	//if (buf.GetLength() > 250)

	esp_now_send(dstMac.data(), buf.GetBufPtr(), buf.GetLength());
	return 0;
}

void ESPNowCore::AddPeer(const std::array<uint8_t, 6>& peerMac)
{
	esp_now_peer_info peerInfo;

	if (esp_now_get_peer(peerMac.data(), &peerInfo) == ESP_ERR_ESPNOW_NOT_FOUND)
	{
		peerInfo = {};
		memcpy(peerInfo.peer_addr, peerMac.data(), 6);
		peerInfo.channel = 1;
		peerInfo.encrypt = false;

		esp_now_add_peer(&peerInfo);
	}
}

void ESPNowCore::BroadcastPing()
{
	std::array<uint8_t, 6> dstMac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t data = 0x42;

	AddPeer(dstMac);

	ESP_ERROR_CHECK(esp_now_send(dstMac.data(), &data, sizeof(data)));
}
