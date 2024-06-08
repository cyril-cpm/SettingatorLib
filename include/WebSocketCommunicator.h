#ifndef _WS_COMMUNICATOR_
#define _WS_COMMUNICATOR_

#include "Communicator.h"

class Message;
class WebSocketsServer;

class WebSocketCTR: public ICTR
{
    public:
    /*
    - create an instance of SerialCTR with baudRate
    */
    static WebSocketCTR* CreateInstance(int port = 8081);

    virtual int     Write(Message& buf) override;
    virtual void    Update() override;

    private:
    WebSocketCTR(int port);

    WebSocketsServer* fWebSocketServer;
};

#endif