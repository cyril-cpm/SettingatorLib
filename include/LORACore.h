#pragma once

#include "Definitions.h"

#if STR_HAS_LORA
#include "esp_err.h"
#include "sdkconfig.h"
#include <cstdint>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <initializer_list>
#include "Buffer.h"
#include "hal/uart_types.h"
#include "Core.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"

class LORACore : public ICore
{
	public:

		static LORACore& GetInstance()
		{
			static LORACore instance(
				static_cast<uart_port_t>(CONFIG_STR_LORA_PORT),
				CONFIG_STR_LORA_TX_PIN,
				CONFIG_STR_LORA_RX_PIN,
				CONFIG_STR_LORA_BAUDRATE,
				CONFIG_STR_LORA_AUX_PIN,
				CONFIG_STR_LORA_M0_PIN,
				CONFIG_STR_LORA_M1_PIN
			);

			return instance;
		}

		int Write(
				uint16_t addr,
				uint8_t channel,
				std::initializer_list<uint8_t> message
			) const {
			if (_WriteHeader(addr, channel) == 0) 
				return 0;
			return uart_write_bytes(fUartPort, message.begin(), message.size());
		}

		int Write(uint16_t addr, uint8_t channel) const {
			if (_WriteHeader(addr, channel) == 0)
				return 0;
			return uart_write_bytes(fUartPort, messageBuffer.data(), messageBuffer.len());
		}

		void Read() {}

		static void LoRaTask(void* parameters) {
			LORACore& LoRaCore = LORACore::GetInstance();

			LoRaCore._Init();
			LoRaCore._Run();
			
		}

		void Init() {
			xTaskCreateStaticPinnedToCore(
					LoRaTask,
					"LoRaTask",
					CONFIG_STR_LORA_TASK_STACK_SIZE,
					NULL,
					20, 
					fLoRaStackBuffer,
					&fLoRaTaskBuffer,
					0
				);

			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			ESP_LOGI("LORACore", "Initialised");
		}

		uint16_t GetAddress() const { return fAddress; }
		void SetAddress(uint16_t address) { fAddress = address; }

		uint8_t GetChannel() const { return fChannel; }
		void SetChannel(uint8_t channel) { fChannel = channel; }

		bool IsModuleBusy() const { return false; }

		void SetMode(uint8_t mode) {}

	private:

		LORACore(uart_port_t port, int tx, int rx, int baudrate,
				int auxPin, int m0Pin, int m1Pin)
			:
				fUartPort(port),
				fRx(rx),
				fTx(tx),
				fBaudrate(baudrate),
				fAuxPin(auxPin),
				fM0Pin(m0Pin),
				fM1Pin(m1Pin),
				fTaskHandle(nullptr),
				fAddress(0),
				fChannel(0)
		{}

		int _WriteHeader(uint16_t addr, uint8_t channel) const {
			uint8_t header[3] = {
				static_cast<uint8_t>(addr >> 8),
				static_cast<uint8_t>(addr & 0xFF),
				channel
			};
			int r = uart_write_bytes(fUartPort, header, 3);
			if (r < 3) {
				ESP_LOGD("LORACore", "_WriteHeader: only %d/3 bytes sent", r);
				return 0;
			}
			return r;
		}

		void _Init() {
			gpio_config_t io_conf = {
				.pin_bit_mask = (1ULL << fM0Pin) | (1ULL << fM1Pin),
				.mode = GPIO_MODE_OUTPUT,
				.pull_up_en = GPIO_PULLUP_ENABLE,
				.pull_down_en = GPIO_PULLDOWN_DISABLE,
				.intr_type = GPIO_INTR_DISABLE
			};
			ESP_ERROR_CHECK(gpio_config(&io_conf));

			gpio_config_t aux_conf = {
				.pin_bit_mask = (1ULL << fAuxPin),
				.mode = GPIO_MODE_INPUT,
				.pull_up_en = GPIO_PULLUP_DISABLE,
				.pull_down_en = GPIO_PULLDOWN_DISABLE,
				.intr_type = GPIO_INTR_DISABLE
			};
			ESP_ERROR_CHECK(gpio_config(&aux_conf));

			uart_config_t uart_conf = {
				.baud_rate = fBaudrate,
				.data_bits = UART_DATA_8_BITS,
				.parity = UART_PARITY_DISABLE,
				.stop_bits = UART_STOP_BITS_1,
				.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
				.rx_flow_ctrl_thresh = 0,
				.source_clk = UART_SCLK_DEFAULT
			};

			ESP_ERROR_CHECK(uart_param_config(fUartPort, &uart_conf));
			ESP_ERROR_CHECK(
					uart_set_pin(
						fUartPort,
						fTx,
						fRx,
						UART_PIN_NO_CHANGE,
						UART_PIN_NO_CHANGE
					)
				);

			ESP_ERROR_CHECK(uart_driver_install(fUartPort, 256, 0, 0, nullptr, 0));

			esp_err_t err = gpio_install_isr_service(0);
			if (err != ESP_ERR_INVALID_STATE)
				ESP_ERROR_CHECK(err);

			// fTaskHandle est membre : le pointeur reste valide pour les ISR futures
			fTaskHandle = xTaskGetCurrentTaskHandle();
			ESP_ERROR_CHECK(gpio_isr_handler_add(
				static_cast<gpio_num_t>(fAuxPin),
				[](void* args) {
					TaskHandle_t h = *static_cast<TaskHandle_t*>(args);
					BaseType_t woken = pdFALSE;
					vTaskNotifyGiveFromISR(h, &woken);
					portYIELD_FROM_ISR(woken);
				},
				&fTaskHandle
			));
			ESP_ERROR_CHECK(gpio_set_intr_type(
				static_cast<gpio_num_t>(fAuxPin), GPIO_INTR_POSEDGE));

			ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(fM0Pin), 1));
			ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(fM1Pin), 1));
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

			uint8_t mac[6];
			ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_BASE));
			fAddress = (static_cast<uint16_t>(mac[4]) << 8) | mac[5];

			// 0xC0 = write permanent, 0x00 = start reg, 0x03 = 3 bytes:
			// ADDH, ADDL, SPED (baud | 8N1 parity | 62.5k air data rate = 0xFF)
			uint8_t cfg[] = {
				0xC0,
				0x00,
				0x03,
				static_cast<uint8_t>(fAddress >> 8),
				static_cast<uint8_t>(fAddress & 0xFF),
				0xFF
			};
			uart_write_bytes(fUartPort, cfg, sizeof(cfg));
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

			if (mainTaskHandle)
				xTaskNotifyGive(mainTaskHandle);
		}

		void _Run() {
			// while (true) {
			// 	// uart_read_bytes(fUartPort, dd, uint32_t length, uint32_t ticks_to_wait)
			// }
		}

		uart_port_t		fUartPort;
		int				fRx;
		int				fTx;
		int				fBaudrate;
		int				fAuxPin;
		int				fM0Pin;
		int				fM1Pin;
		TaskHandle_t	fTaskHandle;

		uint16_t		fAddress;
		uint8_t			fChannel;

		StackType_t			fLoRaStackBuffer[CONFIG_STR_LORA_TASK_STACK_SIZE];
		StaticTask_t		fLoRaTaskBuffer;
};

#endif
