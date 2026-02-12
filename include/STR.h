#pragma once

#include <variant>
#include "ESPNowCore.h"
#include "UARTCore.h"

using CORE_t = std::variant<std::monostate, UARTCore, ESPNowCore>;


