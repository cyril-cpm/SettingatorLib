//#define SERIAL_DEBUG

#include <HP_ESP32Servo.h>
#include <Settingator.h>
#include <ESPNowCommunicator.h>
#include <HardwareSerialCommunicator.h>
#include <MiscDef.h>
//#include <STR.h>
#include "CustomType.h"

#include <WebSocketCommunicator.h>

Servo triggerServo(GPIO_NUM_12);
Servo motorServo(GPIO_NUM_27);

#define TRIGGER_PIN 12
#define MOTOR_PIN 27

bool displayMotorPos = false;

void shoot()
{
  triggerServo.write(95);
  delay(1000);
  triggerServo.write(40);
}

STR_Float motorPos = 0;

/*char* posLabel = (char*)malloc(6);
uint8_t refPosLabel = 0;

void updatePosLabel()
{
    if (displayMotorPos)
    {
        posLabel = itoa(motorPos, posLabel, 10);
        STR.UpdateSetting(refPosLabel, (byte*)posLabel, sizeof(posLabel));
    }
}*/

void updateServoMotor()
{
    motorServo.write(motorPos);
}

void balai()
{
    for (auto i = 0.0f; i <= 180.0f; i+=0.1f)
    {
        motorServo.write(i);
        delay(20);
    }
}

void setup() {
  Serial.begin(115200);

  /*for (int i = 0; i < 3; i++)
    posLabel[i] = '0';

  posLabel[3] = '\0';*/

  triggerServo.write(40);

  motorServo.write(0);

  delay(1000);

  balai();

  //STR.SetCommunicator(ESPNowCTR::CreateInstanceDiscoverableWithSSID("Turret"));

  //INIT_DEFAULT_SETTINGATOR();
  STR.SetCommunicator(HardwareSerialCTR::CreateInstance(115200));
  STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "SHOOT", &shoot);

  /*STR.AddSetting(Setting::Type::Slider, &motorPos, sizeof(motorPos), "POSITION", [](){
    updatePosLabel();
  });
  // TEST MOTOR
  STR.AddSetting(Setting::Type::Switch, &displayMotorPos, sizeof(displayMotorPos), "DISPLAY MOTOR POS");

  STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "+10", [](){
    motorPos += 10;
    if (motorPos > 180)
        motorPos = 180;
    updatePosLabel();
});

  STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "++", [](){ 
    if (motorPos < 180) 
        motorPos++;
    updatePosLabel();
  });

  refPosLabel = STR.AddSetting(Setting::Type::Label, posLabel, sizeof(posLabel));

  STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "--", [](){ 
    if (motorPos > 0)
        motorPos--;
    updatePosLabel();
  });

  STR.AddSetting(Setting::Type::Trigger, nullptr, 0, "-10", [](){
    motorPos -= 10;
    if (motorPos > 180)
        motorPos = 0;
    updatePosLabel();
  });*/
}

void loop () {
  STR.Update();
  updateServoMotor();
}