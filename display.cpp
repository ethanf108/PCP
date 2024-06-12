#include "display.h"
#include <Arduino.h>
#include <RGBmatrixPanel.h>

unsigned short nums[10] = {
  0b1111011011011110,
  0b0010010010010010,
  0b1110011111001110,
  0b1110011110011110,
  0b1011011110010010,
  0b1111001110011110,
  0b1111001111011110,
  0b1110010010010010,
  0b1111011111011110,
  0b1111011110010010
};

#define CLK 11 // USE THIS ON ARDUINO MEGA
#define OE   9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
#define D   A3

RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false, 64);

void display_setup() {
  matrix.begin();
}
