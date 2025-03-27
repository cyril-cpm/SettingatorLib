#include "CustomType.h"
#include "Settingator.h"

BaseCustomType::BaseCustomType(Setting::Type type, void* dataPtr, size_t dataSize, const char* name, void (*callback)())
{
    fRef = STR.AddSetting(type, dataPtr, dataSize,  name, nullptr);
}

void BaseCustomType::SetCallback(void (*callback)())
{
    STR.GetSettingByRef(fRef)->setCallback(callback);
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

STR_Float::STR_Float() : BaseCustomType(Setting::Type::CustomFloat, &fValue, sizeof(fValue),  "CUSTOM FLOAT", nullptr)
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

STR_UInt32::STR_UInt32() : BaseCustomType(Setting::Type::UInt32, &fValue, sizeof(fValue),  "UInt32", nullptr)
{

}

STR_UInt32::STR_UInt32(uint32_t value) : STR_UInt32()
{
    fValue = value;
}

STR_UInt32::operator uint32_t()
{
    return fValue;
}

void STR_UInt32::operator=(uint32_t value)
{
    fValue = value;
    Update();
}

void STR_UInt32::operator+=(uint32_t value)
{
    fValue += value;
    Update();
}

void STR_UInt32::operator-=(uint32_t value)
{
    fValue -= value;
    Update();
}

uint32_t STR_UInt32::operator++()
{
    return ++fValue;
    Update();
}

uint32_t STR_UInt32::operator--()
{
    return --fValue;
    Update();
}

uint32_t STR_UInt32::operator++(int)
{
    return fValue++;
    Update();
}

uint32_t STR_UInt32::operator--(int)
{
    return fValue--;
    Update();
}