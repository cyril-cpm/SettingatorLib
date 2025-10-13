#if defined(BRIDGE)
#include <CommunicatorBridge.h>
#include <UARTCommunicator.h>

CTRBridge* bridge = nullptr;

extern "C" void app_main()
{
bridge = CTRBridge::CreateInstance(UARTCTR::CreateInstance(115200));

bridge->begin();

while (true)
    bridge->Update();

}

#endif