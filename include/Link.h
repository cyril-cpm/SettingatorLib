#pragma once

#include "Definitions.h"

#if STR_HAS_LORA || STR_HAS_ESPNOW
#include "esp_types.h"
#include <span>
#include "Slave.h"

#define SLAVE_BROADCAST_PING 0x01
#define BRIDGE_BROADCAST_PING 0x02
#define LINK_PING 0x03
#define LINK_PONG 0x04
#define SLAVEID_TRANSMISSION_TO_BRIDGE 0x05
#define BRIDGE_TO_SLAVE_HANDSHAKE 0x06

class CORELink
{
	public:

	protected:
	
		void	TreatLinkMessage(
				this auto&& self,
				uint8_t* data,
				uint16_t len,
				std::span<uint8_t> srcAddress,
				uint8_t rssi,
				uint8_t floor,
				uint32_t timestamp
			) {

			using SelfType = std::decay_t<decltype(self)>;

			if constexpr (SelfType::runSlave)
			{
				switch (data[0])
				{
					case SLAVE_BROADCAST_PING:
						if (len == 7)
						{
							ESP_LOGD(SelfType::logTag, "SLAVE_BROADCAST_PING");
							OptSlaveRef slave = GetSlaveForEMac({data[1],
																data[2],
																data[3],
																data[4],
																data[5],
																data[6]});

							if (initEspNowBroadcasted && !slave)
							{
								slave = CreateSlave({data[1],
													data[2],
													data[3],
													data[4],
													data[5],
													data[6]});
							}

							if (slave)
							{
								ESPNowCTR& ctr = slave->get().GetCTR<ESPNowCTR, SLAVE_CTR_ESPNOW>();

								if (!ctr)
								{
									ctr.SetMac(src_addrArr);
									ctr.PlanifyPingTimerCreation();
								}

								ctr.PlanifyBridgeToSlaveHandshake();

								if (slave->get().GetID())
									slave->get().PlanifySendInitRequest();
							}
						}
					break;

				}
			}

			if constexpr (SelfType::runMaster)
			{

			}

			if (data[0] == 0xFF)
				self.WriteToBuffer(data, len);
		}
};

class CTRLink
{
	public:

	private:
};
#endif
