#include "LCD.h"

#define LCD_CS(x) digitalWrite(LCD_CS_PIN, x) 
#define LCD_WR(x) digitalWrite(LCD_WR_PIN, x)
#define LCD_DA(x) digitalWrite(LCD_DA_PIN, x)
#define LCD_DELAY delayMicroseconds(3)

static void sendBuffer(uint8_t *buff, uint8_t len);

uint8_t lcdBuffer[11];

void LCD_refresh(void)
{
  lcdBuffer[0] = 0x40;
  sendBuffer(lcdBuffer, 88);
}

const uint8_t initSequence[5][2] = {{0x0A, 0x40}, {0x06, 0x00}, {0x00, 0x40}, {0x00, 0xC0}, {0x01, 0x00}};

void LCD_init(void)
{
  pinMode(LCD_CS_PIN, OUTPUT);
  pinMode(LCD_WR_PIN, OUTPUT);
  pinMode(LCD_DA_PIN, OUTPUT);
  LCD_CS(1);
  LCD_WR(1);
  LCD_DA(1);
  delay(100);
  memset(lcdBuffer, 0, 11);
  LCD_refresh();
  for (uint8_t i = 0; i < 5; i++) {
    sendBuffer(&initSequence[i][0], 11);
  }
}

const uint8_t digits[10] = {0xAF, 0x06, 0xCB, 0x4F, 0x66, 0x6D, 0xED, 0x27, 0xEF, 0x6F};

void LCD_showNumber(uint16_t n)
{
  uint8_t *pnt;
  uint8_t pos;
  uint8_t d;
  for (pos = 0; pos < 3; pos++) {
    switch (pos) {
      case 0: pnt = &lcdBuffer[4]; break;
      case 1: pnt = &lcdBuffer[7]; break;
      case 2: pnt = &lcdBuffer[3]; break;
      default: return;
    }
    if (pos == 1) { // middle digit, half-bytes are reversed
      *pnt &= ~0xFE;
    } else {
      *pnt &= ~0xEF;
    }
    *pnt |= digits[n % 10];
    n /= 10;
    if (pos == 1) {
      *pnt = (((*pnt) & 0xF0)>>4) | (((*pnt) & 0x0F)<<4);
    }    
  }
}

void LCD_showCalibration(void)
{
  lcdBuffer[3] &= ~0xEF;
  lcdBuffer[7] &= ~0xFE;
  lcdBuffer[4] &= ~0xEF;
  lcdBuffer[3] |= 0xA9; // C
  lcdBuffer[7] |= 0x7E; // A
  lcdBuffer[4] |= 0xA8; // L
}
 
const uint8_t axisMarksByte[4]  = {  6, 5, 2,  1 };
const uint8_t centerMarkByte[4] = { 10, 4, 3, 10 };
const uint8_t centerMarkBit[4]  = {  4, 3, 3,  6 };

void LCD_showTrim(uint8_t channel, int8_t value)
{
  if (value == LCD_TRIM_CLEAR) {
    lcdBuffer[centerMarkByte[channel]] &= ~(0x80 >> centerMarkBit[channel]);
    lcdBuffer[axisMarksByte[channel]] = 0;
    return;
  }
  value = constrain(value, -4, 4);
  if (channel == THROTTLE) 
    value = -value;
  if (value == 0) {
    lcdBuffer[axisMarksByte[channel]] = 0;
    lcdBuffer[centerMarkByte[channel]] |= (0x80 >> centerMarkBit[channel]);
  } else {
    lcdBuffer[centerMarkByte[channel]] &= ~(0x80 >> centerMarkBit[channel]);
    if (value < 0) {
      lcdBuffer[axisMarksByte[channel]] = 0x80 >> (8 + value); 
    } else {
      lcdBuffer[axisMarksByte[channel]] = 0x80 >> (4 - value);
    }
  }
}

const uint8_t batteryMarks[] = {0x04, 0x44, 0xC4, 0xD4, 0xF4, 0xF6};

void LCD_showBattery(uint8_t value)
{
  uint8_t *pnt = &lcdBuffer[8];
  *pnt &= ~0xF6;
  if (value > 5)
    value = 5;
  *pnt |= batteryMarks[value];
}

void LCD_showAxes(uint8_t enable) {
  if (enable)
    lcdBuffer[10] |= 0x01;
  else
    lcdBuffer[10] &= ~0x01;
}

static void sendBit(uint8_t b) 
{
  LCD_DA(b);
  LCD_DELAY;
  LCD_WR(1);
  LCD_DELAY;
  LCD_WR(0);
  LCD_DELAY;
}

static void sendStartBit(void)
{
  LCD_DA(0);
  LCD_CS(1);
  LCD_WR(1);
  LCD_DELAY;
  LCD_CS(0);
  LCD_WR(0);
  LCD_DELAY;
}

static void sendBuffer(uint8_t *buff, uint8_t len)
{
  uint8_t byteCnt = 0;
  uint8_t bitMask = 0x80;
  sendStartBit();
  sendBit(1);
  while (len-- > 0) {
    if (buff[byteCnt] & bitMask)
      sendBit(1);
    else
      sendBit(0);
    bitMask >>= 1;
    if (bitMask == 0) {
      bitMask = 0x80;
      byteCnt++;
    }
  }
}
