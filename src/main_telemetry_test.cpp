#if defined(TELEMETRY_TEST)
#include "Settingator.h"
#include "ESPNowCommunicator.h"

Settingator STR(nullptr);

extern "C" void app_main()
{
    STR.begin();

    ESPNowCore::CreateInstance()->BroadcastPing();

    STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "TEST", [](){});

    while (true)
        STR.Update();

}

#endif
