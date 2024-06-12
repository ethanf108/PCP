#include "output.h"
#include "log.h"
#include <Arduino.h>

void output_setup() {
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  digitalWrite(14, LOW);
  digitalWrite(15, LOW);
  digitalWrite(16, LOW);
  digitalWrite(17, LOW);
  digitalWrite(18, LOW);
  digitalWrite(19, LOW);
  logg(TRACE, "Output setup complete");
}

void safe_output(byte pin) {
  if (OUTPUT_SAFE_DISABLE || pin == 0) {
    loggf(DEBUG, "Safe Output blocked for pin %u", pin);
    return;
  }
  digitalWrite(pin, HIGH);
  loggf(TRACE, "Safe Output started for pin %u", pin);
  delay(SAFE_OUTPUT_DELAY);
  digitalWrite(pin, LOW);
  loggf(TRACE, "Safe Output ended for pin %u", pin);
}

void safe_outputs(byte* pins) {
  if (OUTPUT_SAFE_DISABLE || pins == NULL) {
    logg(TRACE, "Safe Output blocked for pins");
    return;
  }
  
  byte* loop = pins;
  while (*loop != 0) {
    digitalWrite(*loop, HIGH);
    loggf(TRACE, "Safe Output started for pin %u", *loop);
    loop++;
  }
  
  delay(SAFE_OUTPUT_DELAY);
  
  while (*pins != 0) {
    digitalWrite(*pins, LOW);
    loggf(TRACE, "Safe Output ended for pin %u", *pins);
    pins++;
  }
}
