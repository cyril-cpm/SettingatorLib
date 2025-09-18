#include "Settingator.h"
#include "Led.h"
#include "CustomType.hpp"

Settingator STR(nullptr);

STR_UInt8 r(0, "RED");
STR_UInt8 g(0, "GREEN");
STR_UInt8 b(0, "BLUE");

extern "C" void app_main()
{
    STR.begin();

    RGB color(r, g, b);
    FLed.addLeds(GPIO_NUM_0, &color, 1);

    while(true)
    {
        STR.Update();
        color = RGB(r, g, b);
        FLed.show();
    }
}