#include <Stepper.h>
#include <Servo.h>

// Stepper constants
const float RAW_STEP_ANGLE = 5.625;
const float PULL_IN_SPEED = 600;
const float STEP_ANGLE = RAW_STEP_ANGLE / 25;
const float STEPPER_RPM = RAW_STEP_ANGLE * PULL_IN_SPEED / 360;
const int STEPS_PER_REVOLUTION = (360 / STEP_ANGLE);

// Digital IO PINS
const int CUPS_EIGHTH = 0;
const int CUPS_FOURTH = 1;
const int CUPS_HALF = 2;
const int CUPS_FULL = 3;
const int MODE_SWITCH = 4;
const int DISPENSE = 5;
const int TRI_R = 6;
const int TRI_G = 7;
const int TRI_B = 8;
const int SERVO = 9;
const int STEPPER_THREE = 10;
const int STEPPER_ONE = 11;
const int STEPPER_FOUR = 12;
const int STEPPER_TWO = 13;

// Analog IO PINS
const int OUTPUT_CLEAR_SENSOR = A0;
const int DISPENSING_INDICATOR = A1;
const int ERROR_PREVENTION = A2;


// initialize the stepper library on pins 8 through 11:
Stepper auger_stepper(STEPS_PER_REVOLUTION, STEPPER_ONE
, STEPPER_TWO, STEPPER_THREE, STEPPER_FOUR);
Servo paddle_servo;

void setup() {
  // nothing to do inside the setup
  Serial.begin(9600);
  paddle_servo.attach(SERVO);
}

void drive_stepper(float revolutions, Stepper& stepper) {
  stepper.setSpeed(STEPPER_RPM);
  stepper.step(2 * revolutions * STEPS_PER_REVOLUTION);
}

void set_servo_pos(Servo &servo,int pos) {
  servo.write(pos);
  
}

void loop() {
  //bool dispensing = digitalRead(SENSOR);

  bool dispensing = false;

  int pos = 0;
  for (; pos < 180; pos += 5) 
  {
    set_servo_pos(paddle_servo, pos);
    delay(30);
  }
  
  for (; pos > 0; pos -= 5)
  {
    set_servo_pos(paddle_servo, pos);
    delay(30);
  }

  if (!dispensing) {
    drive_stepper(1.0, auger_stepper);
    delay(1000);
  }
}
