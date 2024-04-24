#include <TM1637Display.h>


// external buttons start stop
#define START_BUTTON_PIN 2 
#define STOP_BUTTON_PIN 3
//trademill button panel 
#define TREADMILL_PLUS 4 
#define TREADMILL_START 6
#define TREADMILL_STOP 5
//diaplay and buzzer pins
#define DIO A4
#define CLK A5
#define BUZZER 9
// temperature sensor and fan supply(2596) enable !pin 
#define FAN 13 
#define TEMPERATURE_LOW 11
// MOTOR PINS
#define MOTOR_PIN_1 8
#define MOTOR_PIN_2 7
// PIR MOTION SENSOR
#define PIR 12

#define SLOW_STEP_TICKS 88
#define FAST_STEP_TICKS 53

unsigned long startTime; // Variable to hold the start time
unsigned long endTime;   // Variable to hold the end time

float temperature = 0;
float last_good_temperature = 0;
int sample_age = 0;
int repeat_delay = 0;
int buzzer_enabled = 1;
int buzzer_disable_blink = 0;
int bounce_count = 0;
unsigned long counter_for_pir = 0;
unsigned long ms_counter_for_step_taken = 0;
unsigned long ms_counter_for_motor = 0;
bool activity_flag = false;
enum treadmillStates
{
  BOUNCING_BEFORE_SLOW,
  SLOW,
  BOUNCING_BEFORE_STOP,
  STOP,
  BOUNCING_BEFORE_FAST,
  FAST,
  COOL_DOWN,
  BOUNCING_BEFORE_PIR_INACTIVE,
  PIR_INACTIVE
};
enum fakeStepsMotorStates
{
  MOTOR_STOP,
  MOTOR_ACTIVE,
  IDLE,
};



unsigned char treadmill_state = STOP;
unsigned char fake_steps_machine_state = IDLE;

TM1637Display display(CLK, DIO);

void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(TEMPERATURE_LOW, INPUT_PULLUP);
  pinMode(FAN, OUTPUT);
  pinMode(MOTOR_PIN_1, OUTPUT);
  pinMode(MOTOR_PIN_2, OUTPUT);
  pinMode(PIR, INPUT_PULLUP);
  pinMode(START_BUTTON_PIN, INPUT);
  pinMode(STOP_BUTTON_PIN, INPUT);
  pinMode(TREADMILL_PLUS, OUTPUT);
  pinMode(TREADMILL_START, OUTPUT);
  pinMode(TREADMILL_STOP, OUTPUT);
  digitalWrite(FAN, HIGH);
  display.setBrightness(7, true);
  display.clear();
  display.showNumberDec(0);

  Serial.begin(115200);
}

void treadmillstart() {

  digitalWrite(TREADMILL_STOP, HIGH);
  delay(60);
  digitalWrite(TREADMILL_STOP, LOW);
  delay(200);
  digitalWrite(TREADMILL_START, HIGH);
  delay(60);
  digitalWrite(TREADMILL_START, LOW);
  delay(4000);
  for (int i = 0; i < 10;i++) {
    digitalWrite(TREADMILL_PLUS, HIGH);
    delay(60);
    digitalWrite(TREADMILL_PLUS, LOW);
    delay(150);
  }
  digitalWrite(TREADMILL_PLUS, HIGH);
  delay(60);
  digitalWrite(TREADMILL_PLUS, LOW);
}
void treadmillspeedup() {
  for (int i = 0; i < 13;i++) {
    digitalWrite(TREADMILL_PLUS, HIGH);
    delay(60);
    digitalWrite(TREADMILL_PLUS, LOW);
    delay(150);
  }
  digitalWrite(TREADMILL_PLUS, HIGH);
  delay(60);
  digitalWrite(TREADMILL_PLUS, LOW);
}

void treadmillStop() {
  digitalWrite(TREADMILL_STOP, HIGH);
  delay(100);
  digitalWrite(TREADMILL_STOP, LOW);
  delay(1000);
}

void pirSensor() {
  digitalWrite(MOTOR_PIN_1, digitalRead(PIR));
}

void loop() {

  bool start_button = !digitalRead(START_BUTTON_PIN);
  bool stop_button = !digitalRead(STOP_BUTTON_PIN);
  bool pir_sensor = digitalRead(PIR);
  static long stepstaken = 0;
  static long today_total_steps = 0;
  treadmillProcessing(start_button, stop_button, pir_sensor, stepstaken);
  fakeStepsProcessing(stepstaken, today_total_steps);
  pirSensor();

  delay(10);
}

unsigned long minuetsToTicks(unsigned int minuets) {
  return (((minuets * 60) * 1000) / 10);
}

void fakeStepsProcessing(long& stepstaken, long& today_total_steps) {
  switch (fake_steps_machine_state) {
  case IDLE:
    if (activity_flag)
    {
      if (stepstaken > 0)
      {
        fake_steps_machine_state = MOTOR_ACTIVE;
        ms_counter_for_motor = 0;
        digitalWrite(MOTOR_PIN_2, HIGH);
        startTime = millis();
      }else {
        activity_flag = false; 
      }
    }
    break;
  case MOTOR_ACTIVE:
    ms_counter_for_motor++;
    if (ms_counter_for_motor >= 30)
    {
      digitalWrite(MOTOR_PIN_2, LOW);
      ms_counter_for_motor = 0;
      fake_steps_machine_state = MOTOR_STOP;

      endTime = millis();
      // Serial.print("Time MOTOR ACTIVE: ");
      // Serial.println((endTime - startTime)/10);
      // startTime = millis();

    }
    break;

  case MOTOR_STOP:
    ms_counter_for_motor++;
    if (ms_counter_for_motor >= 28) {
      ms_counter_for_motor = 0;
      stepstaken--;
      today_total_steps++;
      display.showNumberDec(today_total_steps);
      // endTime = millis();
      // Serial.print("Time MOTOR STOP: ");
      // Serial.println((endTime - startTime)/10);
      // startTime = millis();
      if (stepstaken < 1) {
        activity_flag = false;
        fake_steps_machine_state = IDLE;
      }
      else {
        digitalWrite(MOTOR_PIN_2, HIGH);
        fake_steps_machine_state = MOTOR_ACTIVE;
      }
    }

    break;
  }
}


void treadmillProcessing(bool start_button, bool stop_button, bool pir_sensor, long& stepstaken) {

  static unsigned char last_treadmill_state = treadmill_state;
  switch (treadmill_state)
  {
  case STOP:
    // Serial.print("STOP");
    if (start_button) {
      // Serial.print("start button pressed");
      last_treadmill_state = treadmill_state;
      treadmill_state = BOUNCING_BEFORE_SLOW;
      bounce_count = 0;
      ms_counter_for_step_taken = 0;
    }
    break;
  case BOUNCING_BEFORE_SLOW:
    // Serial.print("BOUNCING_BEFORE_SLOW");
    if (start_button) {
      if (bounce_count > 4) {
        digitalWrite(FAN, LOW);
        treadmillstart();
        treadmill_state = SLOW;
        startTime = millis();
      }
      else {
        bounce_count++;
      }
    }
    else {
      treadmill_state = last_treadmill_state;
    }
    break;
  case SLOW:
    // Serial.println("SLOW");
    if (stop_button) {
      // Serial.println("stop button pressed");
      last_treadmill_state = treadmill_state;
      treadmill_state = BOUNCING_BEFORE_STOP;
      bounce_count = 0;
    }
    if (start_button) {
      // Serial.println("start button pressed");
      last_treadmill_state = treadmill_state;
      treadmill_state = BOUNCING_BEFORE_FAST;
      bounce_count = 0;
    }
    if (!pir_sensor) {
      last_treadmill_state = treadmill_state;
      treadmill_state = BOUNCING_BEFORE_PIR_INACTIVE;
      counter_for_pir = 0;
    }
    else {
      ms_counter_for_step_taken++;
    }
    if (ms_counter_for_step_taken >= 88) { //88
      ms_counter_for_step_taken = 0;
      stepstaken++;
    }
    if (stepstaken >= 200) {
      activity_flag = true;
    }

    break;
  case BOUNCING_BEFORE_STOP:
    // Serial.println("BOUNCING_BEFORE_STOP");
    if (stop_button) {
      if (bounce_count > 4) {
        treadmillStop();
        activity_flag = true;
        treadmill_state = COOL_DOWN;
      }
      else {
        bounce_count++;
      }
    }
    else {
      treadmill_state = last_treadmill_state;
    }
    break;
  case BOUNCING_BEFORE_FAST:
    // Serial.print("BOUNCING_BEFORE_FAST");
    if (start_button) {
      if (bounce_count > 4) {
        treadmillspeedup();
        activity_flag = true;
        treadmill_state = FAST;
        // Serial.print("COOL_DOWN");    
      }
      else {
        bounce_count++;
      }
    }
    else {
      treadmill_state = last_treadmill_state;
    }
    break;
  case FAST:
    // Serial.print("FAST");
    if (stop_button) {
      // Serial.println("stop button pressed");
      last_treadmill_state = treadmill_state;
      treadmill_state = BOUNCING_BEFORE_STOP;
      bounce_count = 0;
    }
    if (!pir_sensor) {
      last_treadmill_state = treadmill_state;
      treadmill_state = BOUNCING_BEFORE_PIR_INACTIVE;
      counter_for_pir = 0;
    }
    // else {
    //   ms_counter_for_step_taken++;
    // }
    // if (ms_counter_for_step_taken >= 53) { //88
    //   ms_counter_for_step_taken = 0;
    //   stepstaken++;
    // }
    // if (stepstaken >= 200) {
    //   activity_flag = true;
    // }
    break;
  case COOL_DOWN:
    // Serial.print("COOL_DOWN");    
    if (start_button) {
      // Serial.println("start button pressed from cool down");
      last_treadmill_state = treadmill_state;
      treadmill_state = BOUNCING_BEFORE_SLOW;
      bounce_count = 0;
    }
    if (digitalRead(TEMPERATURE_LOW)) {
      digitalWrite(FAN, HIGH);
      treadmill_state = STOP;
    }
    break;

  case BOUNCING_BEFORE_PIR_INACTIVE:
    if (!pir_sensor) {
      if (counter_for_pir > 2000) {
        treadmillStop();
        activity_flag = true;
        treadmill_state = COOL_DOWN;
      }
      counter_for_pir++;
    }
    else {
      treadmill_state = last_treadmill_state;
    }
    if (stop_button) {
      // Serial.println("stop button pressed");
      last_treadmill_state = treadmill_state;
      treadmill_state = BOUNCING_BEFORE_STOP;
      bounce_count = 0;
    }
    break;
  }
}
