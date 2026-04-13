#pragma once

#include "Definitions.h"
#include "Message.h"
#include "Slave.h"
#include <cstdint>
#include <initializer_list>
#include <variant>

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

class Master
{
	public:
		bool		HasCTR(MasterCTREnum type) {
			return fCTRArray[type].has_value();
		}

		void		InitCTR(ICTR_t&& ctr, MasterCTREnum type) {
			fCTRArray[type].emplace(std::move(ctr));
		}

		void		SetCTRToUse(MasterCTREnum type) {
			fCTRToUse = type;
		}

		void		Write() const {
			const auto& ctrToUse = fCTRArray[fCTRToUse];

			if (ctrToUse)
				ICTR_T_WRITE(*ctrToUse, , );
		}

		void		Write(std::initializer_list<uint8_t> message) const {
			const auto& ctrToUse = fCTRArray[fCTRToUse];

			if (ctrToUse)
				ICTR_T_WRITE(*ctrToUse, message, message);
		}

	private:

		std::array<std::optional<ICTR_t>, MasterCTREnum::MASTER_CTR_MAX> fCTRArray;
		uint8_t		fCTRToUse = 0;
};
