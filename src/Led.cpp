#include "Led.h"

Strip::Strip(gpio_num_t ledPin, RGB* data, size_t dataSize)
{
	fLedPin = ledPin;
	fData = data;
	fDataSize = dataSize;

	rmt_tx_channel_config_t config = ws2812bTxChannelConfig;

	config.gpio_num = fLedPin;

	rmt_new_bytes_encoder(&ws2812bEncoder, &fEncoderHandler);

	ESP_ERROR_CHECK(rmt_new_tx_channel(&config, &fTxHandle));

	rmt_enable(fTxHandle);

}

void Strip::Show()
{
	auto rawDataSize = fDataSize * 3;
	uint8_t rawData[rawDataSize];

	int j = 0;
	for (auto i  = 0; i < fDataSize; i++)
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
}

void Led::addLeds(gpio_num_t ledPin, RGB* data, size_t dataSize)
{
	fStrip.emplace_back(Strip(ledPin, data, dataSize));
}

void Led::show()
{
	for (auto& strip : fStrip)
		strip.Show();
}
