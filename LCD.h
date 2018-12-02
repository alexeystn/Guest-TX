#ifndef LCD_H
#define LCD_H

#include <arduino.h>
#include "types.h"

#define LCD_CS_PIN  5
#define LCD_WR_PIN  6
#define LCD_DA_PIN  7

#define LCD_TRIM_CLEAR  99

void LCD_init(void);
void LCD_refresh(void);
void LCD_showNumber(uint16_t number);
void LCD_showTrim(uint8_t channel, int8_t value);
void LCD_showBattery(uint8_t value);
void LCD_showAxes(uint8_t enable);
void LCD_showCalibration(void);

#endif // LCD_H
