#ifndef _CUSTOM_TYPE_
#define _CUSTOM_TYPE_

#include "Setting.h"

class BaseCustomType
{
    public:

        BaseCustomType(Setting::Type type, void* dataPtr, size_t dataSize, const char* name, void (*callback)());
        void SetCallback(void (*callback)());

        void Update();
        void SetAutoUpdate(bool value);

    protected:
        bool    fAutoUpdate = false;
        setting_ref fRef = 0;

    private:
        BaseCustomType();
};

class STR_Float : public BaseCustomType
{
    public:

        STR_Float();
        STR_Float(float value);
        virtual operator float();
        void operator=(float value);
        void operator+=(float value);
        void operator-=(float value);
        float operator++();
        float operator--();
        float operator++(int);
        float operator--(int);

    private:

        float   fValue = 0.0f;
};

class STR_UInt32 : public BaseCustomType
{
    public:

        STR_UInt32();
        STR_UInt32(uint32_t value);
        virtual operator uint32_t();
        void operator=(uint32_t value);
        void operator+=(uint32_t value);
        void operator-=(uint32_t value);
        uint32_t operator++();
        uint32_t operator--();
        uint32_t operator++(int);
        uint32_t operator--(int);

    private:

        uint32_t   fValue = 0;
};

#endif