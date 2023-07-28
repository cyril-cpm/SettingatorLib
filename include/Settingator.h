#ifndef _SETTINGATOR_
#define _SETTINGATOR_

#include <Arduino.h>
#include "Communicator.h"

class STR
{
    public:

    STR(ICTR* communicator);

    void Update();

    private:

    ICTR* fCommunicator = nullptr;
};

#endif