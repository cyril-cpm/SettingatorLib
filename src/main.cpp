#include <Arduino.h>
#include "Settingator.h"
#include "WebSocketCommunicator.h"

ICTR* communicator = WebSocketCTR::CreateInstance();
STR settingator = STR(communicator);

void setup() {
  pinMode(11, OUTPUT);
  analogWrite(11, 4);
}

void loop() {
  // put your main code here, to run repeatedly:
}