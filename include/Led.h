#pragma once

#include "Definitions.h"

#if HAS_LED_STRIP

#include "driver/rmt_tx.h"
#include <array>
#include <variant>

static const rmt_tx_channel_config_t ws2812bTxChannelConfig = {
    .gpio_num = GPIO_NUM_0,
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = 20 * 1000 * 1000,
    .mem_block_symbols = CONFIG_STR_RMT_MEM_BLOCK_SYMBOLS,
    .trans_queue_depth = 4,
    .intr_priority = 1,
    .flags = {
        .invert_out = false,
        .with_dma = false,
        .allow_pd = false,
		.init_level = 0
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
        .eot_level = 0,
		.queue_nonblocking = 0,
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

template <int STRIP_PIN, uint16_t DATA_LEN>
class Strip
{
    private:

	std::array<RGB, DATA_LEN> fData;

    rmt_channel_handle_t fTxHandle = nullptr;
    rmt_encoder_handle_t fEncoderHandler = nullptr;

    public:

    Strip() {
	    rmt_tx_channel_config_t config = ws2812bTxChannelConfig;

	    config.gpio_num = (gpio_num_t)STRIP_PIN;

	    rmt_new_bytes_encoder(&ws2812bEncoder, &fEncoderHandler);

	    ESP_ERROR_CHECK(rmt_new_tx_channel(&config, &fTxHandle));

	    rmt_enable(fTxHandle);
	};

    void Show() {
	    auto rawDataSize = fData.size() * 3;
	    uint8_t rawData[rawDataSize];

	    int j = 0;
	    for (auto i  = 0; i < fData.size(); i++)
	    {
	        rawData[j] = fData[i].g;
	        j++;
	        rawData[j] = fData[i].r;
	        j++;
			rawData[j] = fData[i].b;
        	j++;
    	}

    	rmt_transmit(fTxHandle, fEncoderHandler, rawData, rawDataSize, &tx_cfg);
    	rmt_tx_wait_all_done(fTxHandle, 10);
	};

	auto&	GetData() { return fData; };


};

class Led
{
    private:

#if CONFIG_STR_HAS_LED_STRIP_0
	Strip<LS_0_PIN, LS_0_LEN> fStrip0;
#endif

#if CONFIG_STR_HAS_LED_STRIP_1
	Strip<LS_1_PIN, LS_1_LEN> fStrip1;
#endif

#if CONFIG_STR_HAS_LED_STRIP_2
	Strip<LS_2_PIN, LS_2_LEN> fStrip2;
#endif

#if CONFIG_STR_HAS_LED_STRIP_3
	Strip<LS_3_PIN, LS_3_LEN> fStrip3;
#endif

    public:
	Led() {};

    void Show() {

#if CONFIG_STR_HAS_LED_STRIP_0
		fStrip0.Show();
#endif

#if CONFIG_STR_HAS_LED_STRIP_1
		fStrip1.Show();
#endif

#if CONFIG_STR_HAS_LED_STRIP_2
		fStrip2.Show();
#endif

#if CONFIG_STR_HAS_LED_STRIP_3
		fStrip3.Show();
#endif

	};

	static Led&	GetInstance() {
		static Led instance;

		return instance;
	};

#if CONFIG_STR_HAS_LED_STRIP_0
	auto&		Strip0() {
		return fStrip0.GetData();
	}
#endif

#if CONFIG_STR_HAS_LED_STRIP_1
	auto&		Strip1() {
		return fStrip1.GetData();
	}
#endif

#if CONFIG_STR_HAS_LED_STRIP_2
	auto&		Strip2() {
		return fStrip2.GetData();
	}
#endif

#if CONFIG_STR_HAS_LED_STRIP_3
	auto&		Strip3() {
		return fStrip3.GetData();
	}
#endif

};

#endif
