#include "CommunicatorBridge.h"
#include "Communicator.h"
#include "Message.h"

CTRBridge* CTRBridge::CreateInstance(ICTR* first, ICTR* second)
{
    return new CTRBridge(first, second);
}

CTRBridge::CTRBridge(ICTR* first, ICTR* second) : fFirst(first), fSecond(second)
{}

void CTRBridge::Update()
{
    if (fFirst->Available())
    {
        fSecond->Write(*fFirst->Read());
        fFirst->Flush();
    }

    if (fSecond->Available())
    {
        fFirst->Write(*fSecond->Read());
        fSecond->Flush();
    }
}