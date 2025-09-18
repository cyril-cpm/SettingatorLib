#pragma once

#include "esp_types.h"

class CommandModule
{
    public:
    CommandModule(const char* name);

    private:
    
}

class CommandHandler
{
public:
    ~CommandHandler();
    void    TreatCommand(char* cmdBuffer);
    char*   GetNextArg(char* buf);

private:
    uint16_t fCmdBufferIndex = 0;
};