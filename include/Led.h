#pragma once

#include "driver/rmt_tx.h"
#include "vector"

static const rmt_tx_channel_config_t ws2812bTxChannelConfig = {
    .gpio_num = GPIO_NUM_0,
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = 20 * 1000 * 1000,
    .mem_block_symbols = STR_RMT_MEM_BLOCK_SYMBOLS,
    .trans_queue_depth = 4,
    .intr_priority = 1,
    .flags = {
        .invert_out = false,
        .with_dma = false,
        .allow_pd = false
    },
};

static const rmt_bytes_encoder_config_t ws2812bEncoder = {
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

static const rmt_transmit_config_t tx_cfg = {
    .loop_count = 0,
    .flags = {
        .eot_level = 0
    }
};

struct RGB
{
    uint8_t g = 0;
    uint8_t r = 0;
    uint8_t b = 0;

    RGB() {}
    RGB(uint8_t ir, uint8_t ig, uint8_t ib) : g(ig), r(ir), b(ib) {}
};

class Strip
{
    private:
    Strip();

    gpio_num_t  fLedPin = GPIO_NUM_0;
    RGB*        fData = nullptr;
    size_t      fDataSize = 0;

    rmt_channel_handle_t fTxHandle = nullptr;
    rmt_encoder_handle_t fEncoderHandler = nullptr;

    public:
    Strip(gpio_num_t ledPin, RGB* data, size_t dataSize);
    void Show();

};

class Led
{
    private:

    std::vector<Strip> fStrip;

    public:
    void addLeds(gpio_num_t ledPin, RGB* data, size_t dataSize);
    void show();

};

static Led FLed;
