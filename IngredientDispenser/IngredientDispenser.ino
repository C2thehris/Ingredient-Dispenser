#include <Stepper.h>
#include <Servo.h>

const float RAW_STEP_ANGLE = 5.625;
const float PULL_IN_SPEED = 600;
const float STEP_ANGLE = RAW_STEP_ANGLE / 25;
const float STEPPER_RPM = 2 * RAW_STEP_ANGLE * PULL_IN_SPEED / 360;
const int STEPS_PER_REVOLUTION = (180 / STEP_ANGLE);

// Digital IO PINS
const int CUPS_EIGHTH = 0;
const int CUPS_FOURTH = 1;
const int CUPS_HALF = 2;
const int CUPS_FULL = 3;
const int CUPS_THIRD = -1;
const int MODE_SWITCH = 4;
const int DISPENSE = 5;
const int TRI_R = 6;
const int TRI_G = 7;
const int TRI_B = 8;
const int SERVO = 9;
const int STEPPER_WHITE = 10;
const int STEPPER_ORANGE = 11;
const int STEPPER_YELLOW = 12;
const int STEPPER_BLUE = 13;

// Analog IO PINS
const int OUTPUT_FORCE_SENSOR = A0;
const int DISPENSING_INDICATOR = A1;
const int ERROR_PREVENTION_PHOTO = A2;

const int DENSITY = 185;
const int WEIGHT_PER_CUP = DENSITY;
const float VOLUME_PER_ROTATION = .15;

const int MEASUREMENT_BUTTONS = 5;
const int PLUS_MULTIPLIER = -1;

bool dispense_state = false;
bool err = true;
bool selected_amount[MEASUREMENT_BUTTONS];

unsigned long last_stir = 0;

// initialize the stepper library on pins 8 through 11:
Stepper auger_stepper(STEPS_PER_REVOLUTION, STEPPER_WHITE,
                      STEPPER_YELLOW, STEPPER_ORANGE, STEPPER_BLUE);
Servo paddle_servo;

void set_LED(bool r_state, bool g_state, bool b_state)
{
  digitalWrite(TRI_R, r_state);
  digitalWrite(TRI_G, g_state);
  digitalWrite(TRI_B, b_state);
}

void setup() {
  Serial.begin(9600);
  paddle_servo.attach(SERVO);
  auger_stepper.setSpeed(STEPPER_RPM);
  pinMode(TRI_R, OUTPUT);
  pinMode(TRI_G, OUTPUT);
  pinMode(TRI_B, OUTPUT);

  pinMode(CUPS_EIGHTH, INPUT);
  pinMode(CUPS_FOURTH, INPUT);
  pinMode(CUPS_HALF, INPUT);
  pinMode(CUPS_FULL, INPUT);
  pinMode(CUPS_THIRD, INPUT);
  pinMode(MODE_SWITCH, INPUT);
  pinMode(DISPENSE, INPUT);
  
  set_LED(HIGH, HIGH, HIGH);
}

bool value_set()
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
  stepper.step(revolutions * STEPS_PER_REVOLUTION);
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

void stir(Servo& servo)
{
  servo.write(180);
  servo.write(0);
}

void dispense(float volume)
{
  drive_stepper(volume / VOLUME_PER_ROTATION, auger_stepper);
}

void read_buttons()
{
  selected_amount[0] = digitalRead(CUPS_EIGHTH);
  selected_amount[1] = digitalRead(CUPS_FOURTH);
  selected_amount[2] = digitalRead(CUPS_THIRD);
  selected_amount[3] = digitalRead(CUPS_HALF);
  selected_amount[4] = digitalRead(CUPS_FULL);
}


void LED_state(int state)
  // -1/NOTB = Error
  //       0 = error not triggered
  //       2 = input + no error: ready to dispense
  //       3 = dispensing
{
  switch (state)
  {
    case 0:
      set_LED(LOW, LOW, LOW);
      break;
    case 2:
      set_LED(LOW, HIGH, LOW);
      break;
    case 3:
      set_LED(LOW, HIGH, HIGH);
      break;
    default:
      set_LED(HIGH, LOW, LOW);
      break;
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
  
  read_buttons();
  bool mode = digitalRead(MODE_SWITCH);
  if (value_set(selected_amount))
  {
    LED_state(2);
  }

  if (dispense_state == !digitalRead(DISPENSE))
  {
    dispense_state = !dispense_state;
    LED_state(3);
    if (value_set())
    {
      int volume = get_volume(selected_amount, mode);
      dispense(volume); 
    }
  }
  
  unsigned long current_time = millis();
  if (current_time - last_stir > 15000 || current_time < last_stir)
  {
    last_stir = current_time;
    stir(paddle_servo);
  }
}
