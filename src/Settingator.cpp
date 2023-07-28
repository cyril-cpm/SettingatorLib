#include "Settingator.h"

STR::STR(ICTR* communicator) : fCommunicator(communicator)
{
    
}

void STR::Update()
{
    fCommunicator->Update();

    
}