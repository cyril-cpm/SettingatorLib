#pragma once

#include "Definitions.h"

#if STR_HAS_LORA || STR_HAS_ESPNOW

#include "Link.h"
#include "SlaveUtils.h"
#include "Slave.h"
#include "Master.h"

#include "esp_types.h"
#include <initializer_list>

template <typename CORETYPE>
void TreatLinkMessage(
		CORETYPE& core,
		const uint8_t* data,
		uint16_t len,
		std::initializer_list<uint8_t> srcAddress,
		bool initBroadcasted,
		int8_t rssi,
		int8_t floor,
		uint32_t timestamp
	)
{
	if constexpr (CORETYPE::runSlave)
	{
		switch (data[0])
		{
			// case SLAVE_BROADCAST_PING:
			// 	if (len == 7)
			// 	{
			// 		ESP_LOGD(CORETYPE::logTag, "SLAVE_BROADCAST_PING");
			// 		OptSlaveRef slave = GetSlaveForEMac(
			// 				{ data[1], data[2], data[3], data[4], data[5], data[6] }
			// 			);
			//
			// 		if (initBroadcasted && !slave)
			// 		{
			// 			slave = CreateSlave(
			// 					{
			// 						data[1],
			// 						data[2],
			// 						data[3],
			// 						data[4],
			// 						data[5],
			// 						data[6]
			// 					}
			// 				);
			// 		}
			//
			// 		if (slave)
			// 		{
			// 			typename CORETYPE::ctrType& ctr = slave->get().GetCTR<
			// 					typename CORETYPE::ctrType,
			// 					CORETYPE::slaveCtrIndex
			// 				>();
			//
			// 			if (!ctr)
			// 			{
			// 				ctr.SetAddress(srcAddress);
			// 				ctr.PlanifyPingTimerCreation();
			// 			}
			// 			ctr.PlanifyBridgeToSlaveHandshake();
			//
			// 			if (slave->get().GetID())
			// 				slave->get().PlanifySendInitRequest();
			// 		}
			// 	}
			// 	break;

			case SLAVE_BROADCAST_PING:
			case SLAVEID_TRANSMISSION_TO_BRIDGE:
				if (len == 8 || len == 7)
				{
					// ESP_LOGD(CORETYPE::logTag, "SLAVEID_TRANSMISSION %d ", data[7]);
					ESP_LOGD(
							CORETYPE::logTag,
							"SLAVEID_TRANSMISSION || SLAVE_BROADCAST_PING"
						); 
					
					OptSlaveRef slave = GetSlaveForEMac(
							{ data[1], data[2], data[3], data[4], data[5], data[6] }
						);

					if (!slave)
					{
						slave = CreateSlave(
								{
									data[1],
									data[2],
									data[3],
									data[4],
									data[5],
									data[6]
								},
								(len == 8 && data[0] == SLAVEID_TRANSMISSION_TO_BRIDGE)
									? data[7]
									: 0
							);
					}
					if (slave)
					{
						typename CORETYPE::ctrType& ctr = slave->get().GetCTR<
								typename CORETYPE::ctrType,
								CORETYPE::slaveCtrIndex
							>();

						if (!ctr)
						{
							ctr.SetAddress(srcAddress);
							ctr.PlanifyPingTimerCreation();
						}

						if (slave->get().GetID())
							slave->get().PlanifySendInitRequest();
					}
				}
				break;

			case LINK_PONG:
				if (len == 7)
				{
					ESP_LOGD(CORETYPE::logTag, "Pong received");
					using OptCTRRef = std::optional<
							std::reference_wrapper<typename CORETYPE::ctrType>
						>; 
					OptCTRRef ctr = CORETYPE::ctrType::GetCTRByAddress(srcAddress);

					if (ctr)
					{
						ESP_LOGD(CORETYPE::logTag, "ctr found");
						ESP_LOGD(CORETYPE::logTag, "registering linkInfo");

						ctr->get().SetLinkInfo(
								rssi,
								floor,
								timestamp
							);

						ctr->get().SetPeerLinkInfo((int8_t)data[1],
													(int8_t)data[2],
													(data[3] << 24) +
													(data[4] << 16) +
													(data[5] << 8) +
													data[6]);
					}

				}
				break;

			break;

		}
	}

	if constexpr (CORETYPE::runMaster)
	{
		switch (data[0])
		{
			case BRIDGE_BROADCAST_PING:
				{
					typename CORETYPE::ctrType& masterCtr = Master::GetInstance().GetCTR<
							typename CORETYPE::ctrType,
							CORETYPE::masterCtrIndex
						>();

					if (masterCtr.HasThisAddress(srcAddress))
					{
						ESP_LOGD(
								CORETYPE::logTag,
								"BRIDGE_BROADCAST_PING from Master"
							);
						masterCtr.PlanifySlaveIDTransmission();
					}
				}
				break;

			case BRIDGE_TO_SLAVE_HANDSHAKE:
				{
					typename CORETYPE::ctrType& ctr = Master::GetInstance().GetCTR<
							typename CORETYPE::ctrType,
							CORETYPE::masterCtrIndex
						>();

					if (!ctr)
						ctr.SetAddress(srcAddress);

					ESP_LOGD(
							CORETYPE::logTag,
							"BRIDGE_TO_SLAVE_HANDSHAKE"
						);
				}
				break;

			case LINK_PING:
				{
					typename CORETYPE::ctrType& ctr = Master::GetInstance().GetCTR<
							typename CORETYPE::ctrType,
							CORETYPE::masterCtrIndex
						>();

					ESP_LOGD(
							CORETYPE::logTag,
							"PING received"
						);

					if (ctr)
					{
						ESP_LOGD(CORETYPE::logTag, "master CTR found");
						ESP_LOGD(CORETYPE::logTag, "Setting LinkInfo");
						ctr.SetLinkInfo(rssi, floor, timestamp);

						ctr.PlanifyPongSending();
					}
				}
				break;
		}
	}

	if (data[0] == 0xFF)
		core.WriteToBuffer(data, len);
}

#endif
