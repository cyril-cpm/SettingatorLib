#pragma once

#include "Definitions.h"

#include <array>
#include <cstdint>
#include "Communicator.h"

#if STR_HAS_ESPNOW
// #include "ESPNowCommunicator.h"
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

		// template <typename CTR_T, uint8_t I>
		// void		SetESPNowCTRMac(std::array<uint8_t, 6>& mac) {
		// 	CTR_T& ctr = GetCTR<CTR_T, I>();
		//
		// 	if (!ctr)
		// 		ctr.SetMac(mac);
		// }

		template <typename CTR_T, uint8_t I>
		CTR_T&			GetCTR() {
			return std::get<I>(GetCTRArray()[I]).get();
		}

	protected:

	std::array<T, N>	fCTRArray;
	uint8_t				fCTRToUse = 0;
};
