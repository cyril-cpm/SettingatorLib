#include "Settingator.h"

STR::STR(ICTR* communicator) : fCommunicator(communicator)
{
    
}

void STR::Update()
{
    fCommunicator->Update();

    
}

void STR::AddSetting(Setting::Type type, byte* data_ptr, size_t data_size, const char* name)
{
    fSettingVector.push_back(Setting(type, data_ptr, data_size, name, fInternalRefCount++));
}