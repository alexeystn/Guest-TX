#include "LCD.h"

uint8_t lcd_buffer[11];

static void send_buffer(uint8_t *buff, uint8_t len);

void LCD_Refresh(void)
{
  lcd_buffer[0] = 0x40;
  send_buffer(lcd_buffer, 88);
}

void LCD_Init(void)
{
  uint8_t init_buffer[5][2] = {{0x0A, 0x40}, {0x06, 0x00}, {0x00, 0x40}, {0x00, 0xC0}, {0x01, 0x00}};
  pinMode(LCD_CS_PIN, OUTPUT);
  pinMode(LCD_WR_PIN, OUTPUT);
  pinMode(LCD_DA_PIN, OUTPUT);
  LCD_CS(1);
  LCD_WR(1);
  LCD_DA(1);
  delay(100);
  memset(lcd_buffer, 0, 11);
  LCD_Refresh();
  for (uint8_t i = 0; i < 5; i++) {
    send_buffer(&init_buffer[i][0], 11);
  }
}

const uint8_t num_mask[10] = {0xAF, 0x06, 0xCB, 0x4F, 0x66, 0x6D, 0xED, 0x27, 0xEF, 0x6F};

void LCD_Show_Number(uint16_t n)
{
  uint8_t *pnt;
  uint8_t pos;
  uint8_t digit;
  for (pos = 0; pos < 3; pos++) {
    switch (pos) {
      case 0: pnt = &lcd_buffer[4]; break;
      case 1: pnt = &lcd_buffer[7]; break;
      case 2: pnt = &lcd_buffer[3]; break;
      default: return;
    }
    if (pos == 1) {
      *pnt &= ~0xFE;
    } else {
      *pnt &= ~0xEF;
    }
    digit = n % 10;
    n /= 10;
    *pnt |= num_mask[digit];
    if (pos == 1) {
      *pnt = (((*pnt)&0xF0)>>4) | (((*pnt)&0x0F)<<4);
    }    
  }
}


void LCD_Show_CAL(void)
{
  lcd_buffer[3] &= ~0xEF;
  lcd_buffer[7] &= ~0xFE;
  lcd_buffer[4] &= ~0xEF;
  lcd_buffer[3] |= 0xA9;
  lcd_buffer[7] |= 0x7E;
  lcd_buffer[4] |= 0xA8;
}
 
const uint8_t ax_byte[4]  = {  6, 5, 2,  1 };
const uint8_t cnt_byte[4] = { 10, 4, 3, 10 };
const uint8_t cnt_bit[4]  = {  4, 3, 3,  6 };

void LCD_Show_Trim(uint8_t ch, int8_t value)
{
  if (value == LCD_CH_CLEAR) {
    lcd_buffer[cnt_byte[ch]] &= ~(0x80 >> cnt_bit[ch]);
    lcd_buffer[ax_byte[ch]] = 0;
    return;
  }
  value = constrain(value, -4, 4);
  if (ch == THROTTLE) value = -value;
  if (value == 0) {
    lcd_buffer[ax_byte[ch]] = 0;
    lcd_buffer[cnt_byte[ch]] |= (0x80 >> cnt_bit[ch]);
  } else {
    lcd_buffer[cnt_byte[ch]] &= ~(0x80 >> cnt_bit[ch]);
    if (value < 0) {
      lcd_buffer[ax_byte[ch]] = 0x80 >> (8 + value); 
    } else {
      lcd_buffer[ax_byte[ch]] = 0x80 >> (4 - value);
    }
  }
}

const uint8_t bat_mask[] = {0x04, 0x06, 0x26, 0x36, 0xB6, 0xF6};

void LCD_Show_Battery(uint8_t bt, uint8_t value)
{
  uint8_t *pnt;
  if (bt == BATTERY_TX)
    pnt = &lcd_buffer[8];
  else
    pnt = &lcd_buffer[9];
  *pnt &= ~0xF6;
  if (value > 5)
    value = 5;
  *pnt |= bat_mask[value];
}

void LCD_Show_Axes(uint8_t enable) {
  if (enable)
    lcd_buffer[10] |= 0x01;
  else
    lcd_buffer[10] &= ~0x01;
  
}

static void send_bit(uint8_t b) 
{
  LCD_DA(b);
  LCD_DELAY;
  LCD_WR(1);
  LCD_DELAY;
  LCD_WR(0);
  LCD_DELAY;
}

static void send_start_bit(void)
{
  LCD_DA(0);
  LCD_CS(1);
  LCD_WR(1);
  LCD_DELAY;
  LCD_CS(0);
  LCD_WR(0);
  LCD_DELAY;
}

static void send_buffer(uint8_t *buff, uint8_t len)
{
  send_start_bit();
  send_bit(1);

  uint8_t byte_cnt = 0;
  uint8_t bit_mask = 0x80;
  while (len-- > 0) {
    if (buff[byte_cnt] & bit_mask)
      send_bit(1);
    else
      send_bit(0);
    bit_mask >>= 1;
    if (bit_mask == 0) {
      bit_mask = 0x80;
      byte_cnt++;
    }
  }
}

