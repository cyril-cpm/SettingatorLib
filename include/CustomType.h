#ifndef _CUSTOM_TYPE_
#define _CUSTOM_TYPE_

#include "Setting.h"

class BaseCustomType : public Setting
{
    public:

        BaseCustomType(Type type, void* dataPtr, size_t dataSize, const char* name, void (*callback)());

        virtual operator float() = 0;
        virtual void operator=(float value) = 0;
        virtual void operator+=(float value) = 0;
        virtual void operator-=(float value) = 0;
        //virtual STR_Float& operator++() = 0;
        //virtual STR_Float& operator--() = 0;

        void Update();

    protected:
        bool    fAutoUpdate = false;

    private:
        BaseCustomType();
};

class STR_Float : public BaseCustomType
{
    public:

        STR_Float();
        STR_Float(float value);
        virtual operator float();
        void operator=(float value) override;
        void operator+=(float value) override;
        void operator-=(float value) override;
        float operator++();
        float operator--();
        float operator++(int);
        float operator--(int);

    private:

        float   fValue = 0.0f;
};

#endif