#include "Settingator.h"
#include "esp_task_wdt.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "UARTCommunicator.h"
#include "CustomType.hpp"

#define LED_PIN GPIO_NUM_8

Settingator STR(nullptr);

STR_UInt8 r(255, "RED");
STR_UInt8 g(0, "GREEN");
STR_UInt8 b(0, "BLUE");

extern "C" void app_main()
{   
    STR.begin(); 
    
    UARTCTR::CreateInstance();
    STR.SetCommunicator(nullptr);



    rmt_tx_channel_config_t txChannelConfig = {
        .gpio_num = LED_PIN,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 20 * 1000 * 1000,
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
        .intr_priority = 1,
        .flags = {
            .invert_out = false,
            .with_dma = false,
            .allow_pd = false
        },
    };

    rmt_channel_handle_t txHandle;

    rmt_bytes_encoder_config_t ws2812bEncoder = {
        .bit0 = {
            .duration0 = 8,
            .level0 = 1,
            .duration1 = 17,
            .level1 = 0
        },

        .bit1 = {
            .duration0 = 16,
            .level0 = 1,
            .duration1 = 9,
            .level1 = 0
        },
        .flags = {
            .msb_first = true
        }
    };

    rmt_encoder_handle_t ws2812bEncoderHandle;
    rmt_new_bytes_encoder(&ws2812bEncoder, &ws2812bEncoderHandle);

    ESP_ERROR_CHECK(rmt_new_tx_channel(&txChannelConfig, &txHandle));

    rmt_enable(txHandle);

    rmt_transmit_config_t tx_cfg = {
        .loop_count = 0,
        .flags = {
            .eot_level = 0
        }
    };

    while (true)
    {
        STR.Update();
        uint8_t rgbData[3] = {g, r, b};
        rmt_transmit(txHandle, ws2812bEncoderHandle, rgbData, sizeof(rgbData), &tx_cfg);
        rmt_tx_wait_all_done(txHandle, 10);
    }
}