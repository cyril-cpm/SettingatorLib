#ifndef _STR_
#define _STR_

/**************** HELPER ********************/


#include "Settingator.h"
#include "WebSocketCommunicator.h"
#include "HTTPServer.h"


#define Settingator_HELPER(X)                       X

#define INIT_WS_WIFI_HTTPSERVER_SETTINGATOR()       Settingator::StartWiFi();\
                                                    HTTPServer* Settingator_HTTPServer = new HTTPServer(8080);\
                                                    STR.SetCommunicator(WebSocketCTR::CreateInstance());

#define INIT_WS_WIFI_SETTINGATOR()                  Settingator::StartWiFi();\
                                                    STR.SetCommunicator(WebSocketCTR::CreateInstance());

#define INIT_DEFAULT_SETTINGATOR()                  INIT_WS_WIFI_HTTPSERVER_SETTINGATOR()

#endif