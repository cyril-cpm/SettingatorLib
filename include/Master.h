#pragma once

#include "Definitions.h"

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <variant>

#include "Message.h"

#if CONFIG_STR_MASTER_ESPNOW
#include "ESPNowCore.h"
#include "ESPNowCommunicator.h"
#endif

#if STR_MASTER_HAS_UART
#include "UARTCore.h"
#include "UARTCommunicator.h"
#endif

enum MasterCTREnum {

#if CONFIG_STR_MASTER_ESPNOW
	MASTER_CTR_ESPNOW,
#endif

#if CONFIG_STR_MASTER_UART0
	MASTER_CTR_UART0,
#endif

#if CONFIG_STR_MASTER_UART1
	MASTER_CTR_UART1,
#endif

#if CONFIG_STR_MASTER_UART2
	MASTER_CTR_UART2,
#endif

	MASTER_CTR_MAX
};


using MasterCTR = CTRVariant<
	STRIP_FIRST_COMMA(
			dummy

#if CONFIG_STR_MASTER_ESPNOW
			,std::reference_wrapper<ESPNowCTR>
#endif

#if STR_MASTER_HAS_UART
			,std::reference_wrapper<UARTCTR>
#endif
		)
	>;

class Master
{
	public:

		static Master& GetInstance() {
			static Master instance;
			return instance;
		}

		Master() : fCTRArray({
				STRIP_FIRST_COMMA(
						dummy

#if CONFIG_STR_MASTER_ESPNOW
						,espNowCtrArray[registeredEspNowCtr++]
#endif

#if CONFIG_STR_MASTER_UART0
						,uartCtrArray[UartCTREnum::UART_CTR_UART0]
#endif

#if CONFIG_STR_MASTER_UART1
						,uartCtrArray[UartCTREnum::UART_CTR_UART1]
#endif

#if CONFIG_STR_MASTER_UART2
						,uartCtrArray[UartCTREnum::UART_CTR_UART2]
#endif
					)
				}) {}

		bool		HasCTR(MasterCTREnum type) {
			return (bool)fCTRArray[type];
		}

#if CONFIG_STR_MASTER_ESPNOW
		ESPNowCTR&	GetESPNowCTR() {
			return std::get<MASTER_CTR_ESPNOW>(fCTRArray[MASTER_CTR_ESPNOW]).get();
		}
#endif

		std::array<MasterCTR, MasterCTREnum::MASTER_CTR_MAX>& GetCTRArray() {
			return fCTRArray;
		}

		void		InitCTR(MasterCTR&& ctr, MasterCTREnum type) {
			ESP_LOGI("MASTER", "Initalizing CTR %d", type);
			// fCTRArray[type].emplace(std::move(ctr));
		}

		void		SetCTRToUse(MasterCTREnum type) {
			fCTRToUse = type;
		}

		void		Write() const {
			const auto& ctrToUse = fCTRArray[fCTRToUse];

			if (ctrToUse)
				ctrToUse.Write();
		}

		void		Write(std::initializer_list<uint8_t> message) const {
			const auto& ctrToUse = fCTRArray[fCTRToUse];

			if (ctrToUse)
				ctrToUse.Write(message);
		}

		void		Update() {
			for (auto& ctr : fCTRArray)
			{
				if (ctr)
					ctr.Update();
			}
		}

	private:

		std::array<MasterCTR, MasterCTREnum::MASTER_CTR_MAX> fCTRArray;
		uint8_t		fCTRToUse = 0;
};

