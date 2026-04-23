#pragma once

#include "sdkconfig.h"

#define STR_HAS_UART (CONFIG_STR_UART0 || CONFIG_STR_UART1 || CONFIG_STR_UART2)

#define STR_HAS_ESPNOW CONFIG_STR_ESPNOW

#if !STR_HAS_UART && !STR_HAS_ESPNOW
// #error "STR has no communication core"
#endif

#define HAS_LED_STRIP (CONFIG_STR_HAS_LED_STRIP_0 || \
						CONFIG_STR_HAS_LED_STRIP_1 || \
						CONFIG_STR_HAS_LED_STRIP_2 || \
						CONFIG_STR_HAS_LED_STRIP_3)
