#pragma once

#if defined(ARDUINO)
#include <Arduino.h>

#elif defined(ESP_PLATFORM)
#include <esp_types.h>
#define HEX 16
#endif

#define LOG(CONTENT, ...) ESP_LOGI(tag, "\n" CONTENT, ##__VA_ARGS__)
