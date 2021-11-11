#include <Stepper.h>
#include <Servo.h>

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
const int CUPS_FULL_PLUS = -1;
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
const int OUTPUT_FORCE_SENSOR = A0;
const int DISPENSING_INDICATOR = A1;
const int ERROR_PREVENTION_PHOTO = A2;

const int DENSITY = 100;
const int WEIGHT_PER_CUP = DENSITY;
const int VOLUME_PER_ROTATION = -1;

const int MEASUREMENT_BUTTONS = 5;
const int PLUS_MULTIPLIER = -1;

bool dispense_state = false;
bool err = true;
bool selected_amount[MEASUREMENT_BUTTONS];


int last_stir = 0;
// initialize the stepper library on pins 8 through 11:
Stepper auger_stepper(STEPS_PER_REVOLUTION, STEPPER_ONE,
                      STEPPER_TWO, STEPPER_THREE, STEPPER_FOUR);
Servo paddle_servo;

void set_LED(bool r_state, bool g_state, bool b_state)
{
  digitalWrite(TRI_R, r_state);
  digitalWrite(TRI_G, g_state);
  digitalWrite(TRI_B, b_state);
}

void setup() {
  // nothing to do inside the setup
  Serial.begin(9600);
  paddle_servo.attach(SERVO);
  auger_stepper.setSpeed(STEPPER_RPM);
  set_LED(HIGH, HIGH, HIGH);
}

bool value_set(bool* selected_amount)
{
  for (int i = 0; i < MEASUREMENT_BUTTONS; ++i)
  {
    if (selected_amount[i])
      return true;
  }
  return false;
}

void drive_stepper(float revolutions, Stepper& stepper)
{
  stepper.step(2 * revolutions * STEPS_PER_REVOLUTION);
}

void set_servo_pos(Servo &servo, int pos)
{
  servo.write(pos);
}

float get_volume(bool* selected_amount, bool by_weight)
{
  float volume = 0;
  float volume_unit;
  
  if (by_weight)
    volume_unit = WEIGHT_PER_CUP;
  else
    volume_unit = 1.0;
  
  if (selected_amount[0]) 
    volume += 1.0/8.0 * volume_unit;
  if (selected_amount[1])
    volume += 1.0/4.0 * volume_unit;
  if (selected_amount[2])
    volume += 1.0/2.0 * volume_unit;
  if (selected_amount[3])
    volume += volume_unit;
  if (selected_amount[4])
    volume += PLUS_MULTIPLIER * volume_unit;
  
  return volume;
}

void stir(const Servo& servo)
{
  
}

void dispense(float volume)
{
  drive_stepper(volume / VOLUME_PER_ROTATION, auger_stepper);
}

void LED_state(int state)
  //0 = error not triggered
  //1 = error triggered, no button pressed
  //2 = button pressed, ready to dispense
  //3 = dispensing
{
  if (state == 0)
  {
    set_LED(LOW, LOW, LOW);
  }
  else if (state == 1)
  {
    set_LED(LOW, LOW, HIGH);
  }
  else if (state == 2)
  {
    set_LED(LOW, HIGH, LOW);
  }
  else if (state == 3)
  {
    set_LED(LOW, HIGH, HIGH);
  }
}    

void loop() {
  int weight = analogRead(OUTPUT_FORCE_SENSOR);
  int err_light = analogRead(ERROR_PREVENTION_PHOTO);
  if ((weight > 2.5) && (err_light < 2.5))
  {
   err = false;
   LED_state(1);
  }
  else
  {
   err = true;
   LED_state(0);
  }
  
  selected_amount[0] = digitalRead(CUPS_EIGHTH);
  selected_amount[1] = digitalRead(CUPS_FOURTH);
  selected_amount[2] = digitalRead(CUPS_HALF);
  selected_amount[3] = digitalRead(CUPS_FULL);
  selected_amount[4] = digitalRead(CUPS_FULL_PLUS);
  bool mode = digitalRead(MODE_SWITCH);
  if (value_set(selected_amount))
  {
    LED_state(2);
  }

  bool enable_dispenser = false;
  if (dispense_state = !digitalRead(DISPENSE))
  {
    enable_dispenser = true;
    LED_state(3);
  }
  dispense_state = digitalRead(DISPENSE);
  
  if (enable_dispenser && value_set(selected_amount))
  {
    int volume = get_volume(selected_amount, mode);
    dispense(volume);
  }
  
  unsigned long current_time = millis();
  if (current_time - last_stir > 15000)
  {
    last_stir = current_time;
    stir(paddle_servo);
  }
}
