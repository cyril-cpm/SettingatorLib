#pragma once

#include "sdkconfig.h"

#define STR_HAS_UART (CONFIG_STR_UART0 || CONFIG_STR_UART1 || CONFIG_STR_UART2)

#define STR_HAS_ESPNOW CONFIG_STR_ESPNOW

#if !STR_HAS_UART && !STR_HAS_ESPNOW
// #error "STR has no communication core"
#endif
