#if defined(BRIDGE_TEST)
#include <CommunicatorBridge.h>
#include <UARTCommunicator.h>

CTRBridge BRIDGE(nullptr);

extern "C" void app_main()
{
BRIDGE.SetMaster(UARTCTR::CreateInstance(115200));

BRIDGE.begin();

while (true)
    BRIDGE.Update();

}

#endif
