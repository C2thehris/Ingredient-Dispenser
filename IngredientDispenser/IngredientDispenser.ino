#include <Stepper.h>
#include <Servo.h>

#define NOT_CONNECTED 0

const float RAW_STEP_ANGLE = 5.625;
const float PULL_IN_SPEED = 600;
const float STEP_ANGLE = RAW_STEP_ANGLE / 25;
const float STEPPER_RPM = 2 * RAW_STEP_ANGLE * PULL_IN_SPEED / 360;
const int STEPS_PER_REVOLUTION = (180 / STEP_ANGLE);

// Digital IO PINS
const int CUPS_EIGHTH = 0;
const int CUPS_FOURTH = 1;
const int CUPS_THIRD = 2;
const int CUPS_HALF = 3;
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

const float DENSITY = 185.0;
const float VOLUME_PER_ROTATION = .138;
const int TRIGGER_WEIGHT = 200;
const int TRIGGER_LIGHT = 200;

const int MEASUREMENT_BUTTONS = 4;
const int PLUS_MULTIPLIER = -1;

bool dispense_state = false;
bool err = true;
bool selected_amount[MEASUREMENT_BUTTONS];
float weights[MEASUREMENT_BUTTONS];

unsigned long last_stir = 0;
bool value_in = false;

// initialize the stepper library on pins 8 through 11:
Stepper auger_stepper(STEPS_PER_REVOLUTION, STEPPER_WHITE,
                      STEPPER_YELLOW, STEPPER_ORANGE, STEPPER_BLUE);
Servo paddle_servo;

void set_LED(bool r_state, bool g_state, bool b_state);
bool value_set();
void drive_stepper(float revolutions, Stepper& stepper);
void set_servo_pos(Servo &servo, int pos);
float get_volume(bool* selected_amount, bool by_weight);
void stir(Servo& servo);
void dispense(float volume);
void read_buttons();
void LED_state(int state);

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
  pinMode(CUPS_THIRD, INPUT);
  pinMode(MODE_SWITCH, INPUT);
  pinMode(DISPENSE, INPUT);

  weights[0] = 50;
  weights[1] = 100;
  weights[2] = 150;
  weights[3] = 200;
  
  set_LED(HIGH, HIGH, HIGH);
}

bool value_set()
{
  for (int i = 0; i < MEASUREMENT_BUTTONS; ++i)
  {
    if (selected_amount[i])
    {
      return true;
    }
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

float get_volume(bool by_weight)
{
  float volume = 0;
  float volume_unit;

  Serial.print("Volume_Unit: ");
  Serial.println(volume_unit);

  if (by_weight)
  {
    if (selected_amount[0])
      volume = weights[0] / DENSITY;
    else if (selected_amount[1])
      volume = weights[1] / DENSITY;
    else if (selected_amount[2])
      volume = weights[1] / DENSITY;
    else if (selected_amount[3])
      volume = weights[3] / DENSITY;
  }
  else
  {
    if (selected_amount[0])
      volume = 1.0 / 8.0;
    else if (selected_amount[1])
      volume = 1.0 / 4.0;
    else if (selected_amount[2])
      volume = 1.0 / 3.0;
    else if (selected_amount[3])
      volume = 1.0 / 2.0;
  }

  return volume;
}

void stir(Servo& servo)
{
  servo.write(180);
  servo.write(0);
}

void dispense(float volume)
{
  Serial.println(volume);
  drive_stepper(-1.0 * volume / VOLUME_PER_ROTATION, auger_stepper);
}

void clear_buttons()
{
  for (int i = 0; i < MEASUREMENT_BUTTONS; ++i)
  {
    selected_amount[i] = false;
  }
}

void read_buttons()
{
  bool temp[4] = {0};
  temp[0] = digitalRead(CUPS_EIGHTH);
  temp[1] = digitalRead(CUPS_FOURTH);
  temp[2] = digitalRead(CUPS_THIRD);
  temp[3] = digitalRead(CUPS_HALF);
  bool conds[MEASUREMENT_BUTTONS];
  conds[0] = (temp[0] && temp[0] != selected_amount[0]);
  conds[1] = (temp[1] && temp[1] != selected_amount[1]);
  conds[2] = (temp[2] && temp[2] != selected_amount[2]);
  conds[3] = (temp[3] && temp[3] != selected_amount[3]);

  
  if (conds[0] || conds[1] || conds[2] || conds[3])
  {
    clear_buttons();
    for (int i = 0; i < MEASUREMENT_BUTTONS; ++i)
    {
      selected_amount[i] = temp[i];
      if (temp[i])
      {
        Serial.print("Button Detected: ");
        Serial.println(i);
        break;
      }
    }
  }
  
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
      set_LED(HIGH, HIGH, HIGH);
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
  read_buttons();
  if (!value_in)
  {
    value_in = value_set();
  }

  if (weight > TRIGGER_WEIGHT)
  {
    if (value_in)
    {
      value_in = true;
      if (digitalRead(DISPENSE))
      {
        bool mode = digitalRead(MODE_SWITCH);
        LED_state(3);
        value_in = false;
        float volume = get_volume(mode);
        dispense(volume);
        delay(20);
        clear_buttons();
      }
      else
      {
        LED_state(2);
      }
    }
    else
    {
      LED_state(1);
    }
  }
  else
  {
    LED_state(0);
  }

  unsigned long current_time = millis();
  if (current_time - last_stir > 15000 || current_time < last_stir)
  {
    last_stir = current_time;
    stir(paddle_servo);
  }
}
