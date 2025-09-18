#include "CommandHandler.h"

CommandHandler::~CommandHandler()
{

}

static bool IsWhiteSpace(char c)
{
    return (c != ' ' && c != '\t' && c != '\n');
}

static bool IsValidChar(char c)
{
    return !IsWhiteSpace(c) && c;
}

void CommandHandler::TreatCommand(char* cmdBuffer)
{
    if (!cmdBuffer)
        return;

    fCmdBufferIndex = 0;
    bool parsing = true;
    while (parsing)
    {
        char* arg = GetNextArg(cmdBuffer);

        if (!arg || !*arg)
            parsing = false;
        else
        {

        }
    }
}

char* CommandHandler::GetNextArg(char* buf)
{
    if (!buf)
        return nullptr;

    for (; !IsWhiteSpace(buf[fCmdBufferIndex]); fCmdBufferIndex++);

    char* ret;
    
    ret = &buf[fCmdBufferIndex];

    for (; IsValidChar(buf[fCmdBufferIndex]); fCmdBufferIndex++);

    return ret;
}