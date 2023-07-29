#include <Arduino.h>
#include "MiscDef.h"
#include "Settingator.h"
#include "Setting.h"
#include "WebSocketCommunicator.h"
#include <HardwareSerial.h>

//ICTR* communicator = WebSocketCTR::CreateInstance();
//STR settingator = STR(communicator);



void setup() {
    Serial.begin(9600);
    Serial.println("\nBegin test:");

    
    uint8_t value = 8;
    String str("Salut maglle");
    Setting test(Setting::Type::Slider, str.begin(), str.length(), "yolo", 4);
    value = 42;

    size_t size = 0;
    byte* buffer = nullptr;

    size = test.getInitRequestSize();

    buffer = (byte*)malloc(size * sizeof(buffer));
    test.getInitRequest(buffer);

    Serial.print("Returned size : ");
    Serial.println(size);

    if (buffer)
    {
        Serial.print("Buffer is : ");
        printBuffer(buffer, size, HEX);
    }
    else
        Serial.println("Buffer is nullptr");

    Serial.println("");
   printBuffer((byte*)"yolo", 4, DEC);
}

void loop() {
  // put your main code here, to run repeatedly:
}