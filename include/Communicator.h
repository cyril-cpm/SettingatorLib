#pragma once

#include "Definitions.h"

#include "Message.h"
#include <initializer_list>
#include <sys/_stdint.h>
#include <queue>
#include <type_traits>
#include <variant>
#include "MiscDef.h"
#include "UARTCore.h"
#include "ESPNowCore.h"
#include <mutex>

class ICTR
{
	public:

	enum LinkType
	{
		ESP_NOW = 0x00,
		UART = 0x01,
		UNKNOWN = 0xFF
	};

uint16_t	GetLinkInfoSize(this auto&& self) {
		return self.GetLinkInfoSizeImpl();
	}

	void		WriteLinkInfoToBuffer(this auto&& self, uint16_t index) {
		self.WriteLinkInfoToBufferImpl(index);
	}

	constexpr explicit operator bool() const { return fActivated; }

	protected:
	
	ICTR() = default;

	bool fActivated = false;

};

template <typename... Ts>
class CTRVariant : public std::variant<Ts ...>
{
	using std::variant<Ts ...>::variant;
	using std::variant<Ts ...>::operator=;

	public:

	constexpr explicit operator bool() const {
		return std::visit([](const auto& ctr) -> bool {
				return (bool)ctr.get();
			}, *this);
	}

	void Write(std::initializer_list<uint8_t> message) const {
		std::visit([message](const auto& ctr) {
				ctr.get().Write(message);
			}, *this);
	}

	void Write() const {
		std::visit([](const auto& ctr) {
				ctr.get().Write();
			}, *this);
	}

	void Update() {
		std::visit([](auto& ctr) {
				ctr.get().Update();
			}, *this);
	}
};
;
