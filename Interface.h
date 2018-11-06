#ifndef INTERFACE_H
#define INTERFACE_H

#include <arduino.h>
#include "types.h"

#define BUTTON_0_PIN  A5
#define BUTTON_1_PIN  A6

#define BEEPER_PIN    3

#define BATTERY_PIN   A4

void HW_Init(void);
uint8_t Button_Pressed(uint8_t n);
void Beep(uint8_t num);

#endif




