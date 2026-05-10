#pragma once

#include "Definitions.h"

#include <array>
#include "Communicator.h"

#if STR_HAS_ESPNOW
#include "ESPNowCommunicator.h"
#endif

#if STR_HAS_UART
#include "UARTCommunicator.h"
#endif

template<typename T, uint8_t N>
class CTRHandler
{
	CTRHandler() = delete;

	public:

	CTRHandler(std::array<T, N>&& ctrList) : fCTRArray(std::move(ctrList)) {}

	public:

		bool		HasCTR(uint8_t type) {
			return (bool)fCTRArray[type];
		}

// #if CONFIG_STR_SLAVE_ESPNOW
// 		ESPNowCTR&	GetESPNowCTR() {
// 			return std::get<std::reference_wrapper<ESPNowCTR>>(fCTRArray[type]).get();
// 		}
// #endif
// #if CONFIG_STR_MASTER_ESPNOW
// 		ESPNowCTR&	GetESPNowCTR() {
// 			return std::get<MASTER_CTR_ESPNOW>(fCTRArray[MASTER_CTR_ESPNOW]).get();
// 		}
// #endif
		ICTR&		GetCTR(uint8_t type) {
			return std::get<type>(fCTRArray[type]).get();
		}

		std::array<T, N>& GetCTRArray() {
			return fCTRArray;
		}

		void		SetCTRToUse(uint8_t type) {
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

	protected:

	std::array<T, N>	fCTRArray;
	uint8_t				fCTRToUse = 0;
};
