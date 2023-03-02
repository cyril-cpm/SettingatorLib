#include <Arduino.h>
#include "Settingator.h"
#include "SerialCommunicator.h"

ICTR* communicator = SerialCTR::CreateInstance(9600);
STR settingator = STR(communicator);

void setup() {
  pinMode(11, OUTPUT);
  analogWrite(11, 4);
}

void loop() {
  // put your main code here, to run repeatedly:
}