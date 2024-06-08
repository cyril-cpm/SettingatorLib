#pragma once

#include "Message.h"

class ICTR;

class CTRBridge
{
    public:

    static CTRBridge*   CreateInstance(ICTR* first, ICTR* second);

    CTRBridge(ICTR* first, ICTR*second);

    void                Update();

    private:

    ICTR*   fFirst = nullptr;
    ICTR*   fSecond = nullptr;
};