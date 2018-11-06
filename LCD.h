#ifndef LCD_H
#define LCD_H

#include <arduino.h>
#include "types.h"

#define LCD_CS_PIN  5
#define LCD_WR_PIN  6
#define LCD_DA_PIN  7

#define LCD_CS(x) digitalWrite(LCD_CS_PIN, x) 
#define LCD_WR(x) digitalWrite(LCD_WR_PIN, x)
#define LCD_DA(x) digitalWrite(LCD_DA_PIN, x)

#define LCD_DELAY delayMicroseconds(3)

#define LCD_CH_CLEAR  99

void LCD_Init(void);
void LCD_Refresh(void);
void LCD_Show_Number(uint16_t n);
void LCD_Show_Trim(uint8_t ch, int8_t value);
void LCD_Show_Battery(uint8_t bt, uint8_t value);
void LCD_Show_Axes(uint8_t enable);
void LCD_Show_CAL(void);

#endif // LCD_H

