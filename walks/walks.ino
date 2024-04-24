#include <Arduino.h>
#include <TM1637Display.h>
#include <OneWire.h>
#include <DallasTemperature.h>
/*
 * libraries to install:
 * TM1637 by Avishay Orpaz Version 1.2.0
 * DallasTemperature by Miles Burton 3.9.0
 */
 // Module connection pins (Digital Pins)
#define DIO 2
#define CLK 3
#define BUZZER 4 
#define ONE_WIRE_BUS 5
#define BUZZER_BUTTON 6
#define TEMPERATURE_LOW 7
#define BUILTIN_LED 13


float temperature = 0;
float last_good_temperature = 0;
int sample_age = 0;
int repeat_delay = 0;
int buzzer_enabled = 1;
int buzzer_disable_blink = 0;

TM1637Display display(CLK, DIO);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

int heatup = 280;
void setup()
{
  pinMode(BUZZER, OUTPUT);
  pinMode(BUZZER_BUTTON, INPUT_PULLUP);
  pinMode(TEMPERATURE_LOW, OUTPUT);
  digitalWrite(TEMPERATURE_LOW, HIGH);
  // digitalWrite(BUZZER, HIGH);


  display.setBrightness(0x0f);
  Serial.begin(9600);
  display.clear();
}


void loop()
{
  sensors.begin();
  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0) * 10;
  Serial.print(" C  ");
  Serial.println(temperature);

  if (digitalRead(BUZZER_BUTTON) == LOW) {
    // buzzer_enabled = 0;
    heatup = 320;
  }

  if (temperature > 0) {
    sample_age = 0;
    last_good_temperature = temperature;
    // if (buzzer_enabled) {
    if (heatup == 280) {
      display.setBrightness(7, true);
      display.showNumberDecEx(last_good_temperature, 0b01000000, 0, 3, 0);
    }
    else {
      if (buzzer_disable_blink) {
        display.setBrightness(7, true);
        display.showNumberDecEx(last_good_temperature, 0b01000000, 0, 3, 0);
        buzzer_disable_blink = 0;
      }
      else {
        display.setBrightness(2, true);
        display.showNumberDecEx(last_good_temperature, 0b01000000, 0, 3, 0);
        buzzer_disable_blink = 1;
      }
    }
  }
  else {
    sample_age++;
    if (sample_age > 3) {
      display.setBrightness(1, true);
      display.showNumberDecEx(last_good_temperature, 0b01000000, 0, 3, 0);
    }
  }

  if (sample_age > 120) {
    if (last_good_temperature > 350) {
      // no readings since last 2 minutes
      if (buzzer_enabled) {
      digitalWrite(BUZZER, HIGH);
      delay(1000);
      digitalWrite(BUZZER, LOW);
      delay(300);
      digitalWrite(BUZZER, HIGH);
      delay(1000);
      digitalWrite(BUZZER, LOW);
      delay(300);
      digitalWrite(BUZZER, HIGH);
      delay(1000);
      digitalWrite(BUZZER, LOW);
      delay(300);
      digitalWrite(BUZZER, HIGH);
      delay(1000);
      digitalWrite(BUZZER, LOW);
      }
      sample_age = 35;
    }
  }

  if (last_good_temperature >= heatup) {
    digitalWrite(TEMPERATURE_LOW, LOW);
  }
  else {
    digitalWrite(TEMPERATURE_LOW, HIGH);
  }

  if (last_good_temperature >= 430) {
    if (repeat_delay <= 0) {
      repeat_delay = 2;
    }
  }
  else {
    if (last_good_temperature >= 410) {
      if (repeat_delay <= 0) {
        repeat_delay = 20;
      }
    }
    else {
      if (last_good_temperature >= 390) {
        if (repeat_delay <= 0) {
          repeat_delay = 60;
        }
      }
    }
  }

  if (repeat_delay > 0) {
    repeat_delay--;
    if (repeat_delay == 0) {
      digitalWrite(BUILTIN_LED, HIGH);
      if (buzzer_enabled) {
        digitalWrite(BUZZER, HIGH);
        delay(300);
        digitalWrite(BUZZER, LOW);
      }
    }
  }
  delay(1000);
}
