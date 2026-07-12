#pragma once

#include "Definitions.h"

#include "Message.h"
#include <initializer_list>
#include <sys/_stdint.h>
#include <queue>
#include <type_traits>
#include <variant>
#include "MiscDef.h"
#include <mutex>

class ICTR
{
	public:

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
