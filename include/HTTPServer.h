#include <Arduino.h>

class AsyncWebServer;

class HTTPServer
{
public:
    HTTPServer(uint16_t port);
    ~HTTPServer();

private:
    AsyncWebServer *fServer;
};