#if defined(ESP_PLATFORM)

#include "UARTCommunicator.h"
#include <stdlib.h>
#include <cstring>
#include <esp_log.h>
#include "Message.h"

UARTCTR* UARTCTR::CreateInstance(int baudrate)
{
    ESP_LOGI("UARTCTR", "Creating Instance");
    return new UARTCTR(baudrate);
}

UARTCTR::UARTCTR(int baudrate)
{
    uart_config_t uartConfig = {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
    };

    ESP_LOGI("UARTCTR", "uart_param_config");
    ESP_ERROR_CHECK(uart_param_config(fUartPort, &uartConfig));
    ESP_LOGI("UARTCTR", "Done");

    ESP_LOGI("UARTCTR", "uart_set_pin");
    ESP_ERROR_CHECK(uart_set_pin(fUartPort, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                                            UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_LOGI("UARTCTR", "Done");

    ESP_LOGI("UARTCTR", "uart_driver_install");
    ESP_ERROR_CHECK(uart_driver_install(fUartPort, fRxBufferSize, fTxBufferSize,
                                        0, nullptr, 0));
    ESP_LOGI("UARCTR", "Done");
}

int UARTCTR::Write(Message& buf)
{
    esp_err_t err = uart_write_bytes(fUartPort, buf.GetBufPtr(), buf.GetLength());

    if (err == ESP_FAIL)
    {
        ESP_LOGE("UART", "Write failed");
    }

    return 0;
}

void UARTCTR::Update()
{
    size_t bufferDataLen = 0;

    ESP_ERROR_CHECK(uart_get_buffered_data_len(fUartPort, &bufferDataLen));

    if (bufferDataLen)
    {
        uint8_t* newBuffer = (uint8_t*)malloc(fUartBufferSize + bufferDataLen);

        if (fUartBufferSize)
        {
            memcpy(newBuffer, fUartBuffer, fUartBufferSize);
            free(fUartBuffer);
        }

        uart_read_bytes(fUartPort, newBuffer + fUartBufferSize, bufferDataLen, 0);

        fUartBuffer = newBuffer;
        fUartBufferSize += bufferDataLen;

        int i = 0;
        for (i = 0; i < fUartBufferSize && fUartBuffer[i] != Message::Frame::Start; i++);

        if (i != 0)
            _removeBufferBeginBytes(i);

        if (fUartBufferSize >= 5)
        {
            size_t msgSize = (fUartBuffer[1] << 8) + fUartBufferSize;

            if (fUartBufferSize >= msgSize)
            {
                _receive(new Message(fUartBuffer, msgSize));
                _removeBufferBeginBytes(msgSize);
            }
        }

    }
}

void UARTCTR::_removeBufferBeginBytes(size_t numberBytes)
{
    uint8_t* newBuffer = (uint8_t*)malloc(fUartBufferSize - numberBytes);

    memcpy(newBuffer, fUartBuffer + numberBytes, fUartBufferSize - numberBytes);
    free(fUartBuffer);

    fUartBufferSize = fUartBufferSize - numberBytes;
}

#endif