#include "HTTPServer.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

HTTPServer::HTTPServer(uint16_t port)
{
    if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
      }

    fServer = new AsyncWebServer(port);

    fServer->on("/*", HTTP_GET, [](AsyncWebServerRequest *request) {
      Serial.println("http request received");
      Serial.println(request->url());
      Serial.println(request->host());
      Serial.println(request->contentType());
      request->send(SPIFFS, "/index.html", "text/html");
    });

    Serial.println("Begin Web Server");
    Serial.print("port: ");
    Serial.println(port);
    fServer->begin();
    Serial.println("finish");
}

HTTPServer::~HTTPServer()
{
    delete fServer;
}