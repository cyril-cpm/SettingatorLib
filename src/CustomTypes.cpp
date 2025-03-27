#include "CustomType.h"
#include "Settingator.h"

BaseCustomType::BaseCustomType(Setting::Type type, void* dataPtr, size_t dataSize, const char* name, void (*callback)())
{
    fRef = STR.AddSetting(type, dataPtr, dataSize,  "CUSTOM FLOAT", nullptr);
}

void BaseCustomType::Update()
{
    if (fAutoUpdate)
        STR.SendUpdateMessage(fRef);
}

void BaseCustomType::SetAutoUpdate(bool value)
{
    fAutoUpdate = value;
}

STR_Float::STR_Float() : BaseCustomType(Setting::Type::CustomFloat, &fValue, sizeof(fValue),  "CUSTOM FLOAD", nullptr)
{

}

STR_Float::STR_Float(float value) : STR_Float()
{
    fValue = value;
}

STR_Float::operator float()
{
    return fValue;
}

void STR_Float::operator=(float value)
{
    fValue = value;
    Update();
}

void STR_Float::operator+=(float value)
{
    fValue += value;
    Update();
}

void STR_Float::operator-=(float value)
{
    fValue -= value;
    Update();
}

float STR_Float::operator++()
{
    return ++fValue;
    Update();
}

float STR_Float::operator--()
{
    return --fValue;
    Update();
}

float STR_Float::operator++(int)
{
    return fValue++;
    Update();
}

float STR_Float::operator--(int)
{
    return fValue--;
    Update();
}