#pragma once

#include <esp_types.h>
#include <esp_log.h>

#define HEX 16

#define LOG(CONTENT, ...) ESP_LOGI(tag,  CONTENT, ##__VA_ARGS__)

#define LOG_BUFFER_HEX(BUFFER, SIZE) ESP_LOG_BUFFER_HEX(tag, BUFFER, SIZE);
