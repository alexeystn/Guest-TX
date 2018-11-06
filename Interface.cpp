#include "Interface.h"

void HW_Init(void)
{
  Serial.begin(9600);
  pinMode(BUTTON_0_PIN, INPUT);
  pinMode(BUTTON_1_PIN, INPUT);
}

uint8_t Button_Pressed(uint8_t n)
{
  if (n == 0)
    return !(digitalRead(BUTTON_0_PIN));
  else
    return !(digitalRead(BUTTON_1_PIN));
}

void Beep(uint8_t num)
{
  for (uint8_t i = 0; i < num; i++) {
    analogWrite(BEEPER_PIN, 100);
    delay(200);
    analogWrite(BEEPER_PIN, 0);
    delay(200);    
  }
}










