#include "CustomType.h"
#include "Settingator.h"

BaseCustomType::BaseCustomType(Type type, void* dataPtr, size_t dataSize, const char* name, void (*callback)()) :
    Setting(Setting::Type::CustomFloat, dataPtr, dataSize,  "CUSTOM FLOAD", nullptr, STR.settingRefCount())
{

}

void BaseCustomType::Update()
{
    if (fAutoUpdate)
        STR.SendUpdateMessage(this);
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
}

void STR_Float::operator+=(float value)
{
    fValue += value;
}

void STR_Float::operator-=(float value)
{
    fValue -= value;
}

float STR_Float::operator++()
{
    return ++fValue;
}

float STR_Float::operator--()
{
    return --fValue;
}

float STR_Float::operator++(int)
{
    return fValue++;
}

float STR_Float::operator--(int)
{
    return fValue--;
}