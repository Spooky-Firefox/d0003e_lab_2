#pragma once
#include <stdint.h>

// Enables the lcd and sets up its parameters
void setupLCD();

// toggels the s1 segment
void toggle_s1();

// toggels the s9 segment
void toggle_s9();

// toggels the s4 segment
void toggle_s4();

// the function defined in part one of the lab
// 0 no error
// 1 pos out of range
uint8_t writeChar(char ch, uint8_t pos);

// writes a long to the display
void writeLong(long i);
