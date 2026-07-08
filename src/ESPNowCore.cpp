#include "Definitions.h"

#if STR_HAS_ESPNOW
#include "Core.h"
#include "ESPNowCore.h"
#include "ESPNowCommunicator.h"
#include "STR.h"
#include "MiscDef.h"
#include "Slave.h"
#include "LinkUtils.h"

#include <functional>
#include <esp_wifi.h>
#include <esp_mac.h>
#include <nvs_flash.h>
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include <cstdint>
#include <sys/syslimits.h>


#if CONFIG_STR_SLAVE_ESPNOW
#endif

#if CONFIG_STR_MASTER_ESPNOW
#include "Master.h"
#include "Settingator.h"

static Master& master = Master::GetInstance();
#endif

static const char* tag("ESPNowCore");

static bool isBroadcastMac(std::array<uint8_t, 6>& dstMac)
{
	return (dstMac == std::to_array<uint8_t>({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}));
}

void ESPNowCore::receiveCallback(const esp_now_recv_info* info, const uint8_t* data, int len)
{
	if (info)
	 {
		 std::array<uint8_t, 6> srcAddress;
		if (info->src_addr)
			std::copy(info->src_addr, info->src_addr + 6, srcAddress.begin());

		ESP_LOGD("ESPNowCore", "len: %d", len);
		ESP_LOGD("ESPNowCore", "data: %d", *data);


	TreatLinkMessage<ESPNowCore>(
			ESPNowCore::GetInstance(), 
			data,
			len,
			{
				info->src_addr[0],
				info->src_addr[1],
				info->src_addr[2],
				info->src_addr[3],
				info->src_addr[4],
				info->src_addr[5]
			},
			(int8_t)info->rx_ctrl->rssi,
			(int8_t)info->rx_ctrl->noise_floor,
			info->rx_ctrl->timestamp / 1000
		);
	}
}

void ESPNowCore::Init()
{
	LOG("InitImpl");

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
	ESP_ERROR_CHECK(
			esp_now_register_send_cb(
				[](const esp_now_send_info_t* tx_info, esp_now_send_status_t status) {
					ESPNowCore::GetInstance().SemGive();
					if (status != ESP_NOW_SEND_SUCCESS)
						ESP_LOGE("ESPNowCore", "Sending data failed");


				}
			)
		);

	ESP_ERROR_CHECK(esp_read_mac(fMac.data(), ESP_MAC_WIFI_STA));
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

#endif
