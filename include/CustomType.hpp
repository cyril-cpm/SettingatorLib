#ifndef _CUSTOM_TYPE_
#define _CUSTOM_TYPE_

#include "Settingator.h"
#include "Setting.h"

template <typename T>
class STR_Type
{
    public:

        STR_Type(T value, const char* name = nullptr)
        {
            Setting::Type   type;
            void*           dataPtr     = &fValue;
            int             dataSize    = sizeof(fValue);
        
            if constexpr (std::is_same_v<T, uint8_t>)
                type = Setting::Type::UInt8;
            else if constexpr (std::is_same_v<T, uint16_t>)
                type = Setting::Type::UInt16;
            else if constexpr (std::is_same_v<T, uint32_t>)
                type = Setting::Type::UInt32;
            else if constexpr (std::is_same_v<T, float>)
                type = Setting::Type::Float;
            else if constexpr (std::is_same_v<T, bool>)
                type = Setting::Type::Bool;
        
            fRef = STR.AddSetting(type, dataPtr, dataSize,  name, nullptr);
        }

        void SetCallback(void (*callback)())
        {
            Setting* setting = STR.GetSettingByRef(fRef);
            
            if (setting)
                setting->setCallback(callback);
        }

        void Update()
        {
            if (fAutoUpdate)
                STR.SendUpdateMessage(fRef);
        }

        void SetAutoUpdate(bool value)
        {
            fAutoUpdate = value;
        }

        operator T()
        {
            return fValue;
        }

        void    operator=(T value)
        {
            fValue = value;
            Update();
        }

        void    operator+=(T value)
        {
            fValue += value;
            Update();
        }

        void    operator-=(T value)
        {
            fValue -= value;
            Update();
        }

        T       operator++()
        {
            return ++fValue;
            Update();
        }

        T       operator--()
        {
            return --fValue;
            Update();
        }

        T       operator++(int)
        {
            return fValue++;
            Update();
        }

        T       operator--(int)
        {
            return fValue--;
            Update();
        }

    protected:
        bool    fAutoUpdate = false;
        setting_ref fRef = 0;

        T           fValue;

    private:
        STR_Type();
};


typedef STR_Type<float> STR_Float;
typedef STR_Type<bool> STR_Bool;
typedef STR_Type<uint8_t> STR_UInt8;
typedef STR_Type<uint16_t> STR_UInt16;
typedef STR_Type<uint32_t> STR_UInt32;

#endif