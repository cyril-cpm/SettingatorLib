#pragma once

#include <esp_types.h>
#define HEX 16

#define LOG(CONTENT, ...) ESP_LOGI(tag, "\n" CONTENT, ##__VA_ARGS__)
