#include <Arduino.h>


#ifndef __PCP_OUTPUT_H
#define __PCP_OUTPUT_H

// Millis for safe output for all outputs except flippers
#define SAFE_OUTPUT_DELAY 50
#define SAFE_OUTPUT_COOLDOWN 200
#define OUTPUT_SAFE_DISABLE false

void output_setup(void);
void safe_output(byte pin);
void safe_outputs(byte* pins);

#define OUTPUT_flip1(x) digitalWrite(16, x ? HIGH : LOW)
#define OUTPUT_flip2(x) digitalWrite(17, x ? HIGH : LOW)
#define OUTPUT_trough 18
#define OUTPUT_pop1   3
#define OUTPUT_pop2   5
#define OUTPUT_pop3   6
#define OUTPUT_drop1  7
#define OUTPUT_drop2  4
#define OUTPUT_kick   19
#define OUTPUT_sling1 15
#define OUTPUT_sling2 14
#define OUTPUT_scoop  2

#endif
