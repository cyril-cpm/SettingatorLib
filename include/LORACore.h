#pragma once

#include "Definitions.h"

#if STR_HAS_LORA
#include "Buffer.h"
#include "Core.h"
#include "Link.h"
#include "UARTUtils.h"

#include "esp_err.h"
#include "sdkconfig.h"
#include <cstdint>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <initializer_list>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"

class LORACTR;

class LORACore : public ICore
{
	public:

		static constexpr bool runSlave = CONFIG_STR_SLAVE_LORA;
		static constexpr bool runMaster = CONFIG_STR_MASTER_LORA;
		static constexpr const char*	logTag = "LORACore";
		static constexpr uint8_t slaveCtrIndex = SLAVE_CTR_LORA;
		static constexpr uint8_t masterCtrIndex = MASTER_CTR_LORA;
		using ctrType = LORACTR;
		
		struct LinkInfo {
			int8_t rssi;
			int32_t timestamp;
		};

		enum Frame
		{
			Start = 0x42,
			End = 0x24
		};

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
	
			uint16_t size = message.size() + 6;
			uint8_t begin[] = {
						Frame::Start,
						(uint8_t)(size >> 8),
						(uint8_t)size,
						(uint8_t)(fAddress >> 8),
						(uint8_t)fAddress
					};

			uart_write_bytes(fUartPort, begin, 5);

			uart_write_bytes(fUartPort, message.begin(), message.size());
	
			uint8_t end[] = { Frame::End };
			uart_write_bytes(fUartPort, end, 1);

			return size;
	}

		int Write(uint16_t addr, uint8_t channel) const {
			if (_WriteHeader(addr, channel) == 0)
				return 0;

			uint16_t size = messageBuffer.len() + 6;

			uint8_t begin[] = {
						Frame::Start,
						(uint8_t)(size >> 8),
						(uint8_t)size,
						(uint8_t)(fAddress >> 8),
						(uint8_t)fAddress
					};

			uart_write_bytes(fUartPort, begin, 5);

			uart_write_bytes(fUartPort, messageBuffer.data(), messageBuffer.len());

			uint8_t end[] = { Frame::End };
			uart_write_bytes(fUartPort, end, 1);

			return size;
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

		void ConfigModule() const {
			ESP_ERROR_CHECK(uart_wait_tx_done(fUartPort, portMAX_DELAY));

			ESP_ERROR_CHECK(uart_flush_input(fUartPort));

			ESP_ERROR_CHECK(uart_set_baudrate(fUartPort, 9600));

			ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(fM0Pin), 1));
			ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(fM1Pin), 1));
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

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

			ESP_ERROR_CHECK(uart_flush_input(fUartPort));

			ESP_ERROR_CHECK(uart_set_baudrate(fUartPort, fBaudrate));

			ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(fM0Pin), 0));
			ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(fM1Pin), 0));
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		}

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
			ESP_ERROR_CHECK(uart_driver_install(fUartPort, 256, 0, 0, nullptr, 0));

			uart_config_t uart_conf = {
				.baud_rate = fBaudrate,
				.data_bits = UART_DATA_8_BITS,
				.parity = UART_PARITY_DISABLE,
				.stop_bits = UART_STOP_BITS_1,
				.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
				.rx_flow_ctrl_thresh = 0,
				.source_clk = UART_SCLK_DEFAULT,
				.flags = { 
					.allow_pd = 0,
					.backup_before_sleep = 0
				}
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

			uint8_t mac[6];
			ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_BASE));
			fAddress = (static_cast<uint16_t>(mac[4]) << 8) | mac[5];

			ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(fM0Pin), 0));
			ESP_ERROR_CHECK(gpio_set_level(static_cast<gpio_num_t>(fM1Pin), 0));
			ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

			if (mainTaskHandle)
				xTaskNotifyGive(mainTaskHandle);
		}

		inline void	_ReadUart() {
			ReadUart(fUartPort, fMidBuf);
		}

		inline bool _FetchMessage() {
			return fMidBuf.Fetch(
					LORACore::Frame::Start,
					LORACore::Frame::End,
					"LORACore"
				);
		}

		void _Run();

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

		StackType_t		fLoRaStackBuffer[CONFIG_STR_LORA_TASK_STACK_SIZE];
		StaticTask_t	fLoRaTaskBuffer;

		CircularBuffer	fMidBuf;
};

#endif
