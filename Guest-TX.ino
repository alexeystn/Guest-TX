
#include "types.h"
#include "LCD.h"
#include "PPM.h"
#include "EEPROM.h"
#include "Interface.h"

#define FRAMERATE 40

#define PITCH_PIN     A0
#define ROLL_PIN      A1
#define YAW_PIN       A2
#define THROTTLE_PIN  A3

uint8_t adc_inputs[4] = {ROLL_PIN, PITCH_PIN, YAW_PIN, THROTTLE_PIN};

int16_t adc_raw[4];
int16_t adc_prev[4];

int16_t calib_min[4];
int16_t calib_mid[4];
int16_t calib_max[4];

int8_t  trims[4];
int16_t percent[4];
int16_t ppm[PPM_CH_NUM];

uint8_t i = 0;
uint8_t j = 0;

void setup()
{
  HW_Init();
  PPM_Init();
  LCD_Init();
  
  if (Button_Pressed(0)) {
    LCD_Show_CAL();
    LCD_Refresh();
    Beep(3);
    while (Button_Pressed(0)) delay(100);
    while (!Button_Pressed(0)) {
      static uint8_t b = 0;
      LCD_Show_Axes(!b);
      for (i = 0; i < 4; i++)
        LCD_Show_Trim(i, LCD_CH_CLEAR*b);
      b ^= 1;
      delay(200);  
      LCD_Refresh();
    }
    while (Button_Pressed(0)) delay(100);
   
    for (i = 0; i < 4; i++)
      calib_mid[i] = ADC_Conversion(i);

    Serial_Print_Array("MID", calib_mid, 4);    
    Beep(3);
     
    LCD_Show_Axes(1);
    while (!Button_Pressed(0)) {
      uint16_t adc_value;
      static uint8_t t = 0;
      int8_t s;
      s = (t + (16-2*t)*(t>8)) - 4;
 
      for (i = 0; i < 4; i++) {
        if ((i == ROLL) || (i == THROTTLE)) s = -s;
        LCD_Show_Trim(i, s);
      }
      t++;
      if (t == 16) t = 0;
      LCD_Refresh();
      delay(100); 
      for (i = 0; i < 4; i++) {
        adc_value = ADC_Conversion(i);
        if (adc_value > calib_max[i])
          calib_max[i] = adc_value;
        if (adc_value < calib_min[i])
          calib_min[i] = adc_value;
      }
    }
    while (Button_Pressed(0)) delay(100);

    for (i = 0; i < 4; i++) {
      calib_min[i] = calib_min[i] + (calib_mid[i] - calib_min[i])/10;
      calib_max[i] = calib_max[i] - (calib_max[i] - calib_mid[i])/10;
    }
    Serial_Print_Array("MIN", calib_min, 4);
    Serial_Print_Array("MAX", calib_max, 4);
    Save_to_EEPROM();
    Beep(3);
  } else {
    Beep(4);
  }

  
  Load_from_EEPROM();
  Serial_Print_Array("MIN", calib_min, 4);
  Serial_Print_Array("MID", calib_mid, 4);
  Serial_Print_Array("MAX", calib_max, 4);
  
  Serial.print("Starting... \n");
  
  LCD_Show_Axes(1);
  
}








uint16_t ADC_Conversion(uint8_t ch) {
    if ((ch == ROLL) || (ch == THROTTLE))
      return (analogRead(adc_inputs[ch]));
    else 
      return (1023 - analogRead(adc_inputs[ch]));
}





void loop()
{
  for (i = 0; i < 4; i++) {
    adc_prev[i] = adc_raw[i];
    adc_raw[i] = ADC_Conversion(i);

    if (adc_raw[i] < calib_mid[i]) {
      percent[i] = map(adc_raw[i], calib_min[i], calib_mid[i],    0,   50);
      ppm[i]     = map(adc_raw[i], calib_min[i], calib_mid[i], 1000, 1500);
    } else {
      percent[i] = map(adc_raw[i], calib_mid[i], calib_max[i],   50,  100);
      ppm[i]     = map(adc_raw[i], calib_mid[i], calib_max[i], 1500, 2000);
    }
    trims[i] = map(ppm[i], 1000, 2000, -4, 4);

    trims[i] = constrain(trims[i], -4, 4);
    percent[i] = constrain(percent[i], 0, 100);
    ppm[i] = constrain(ppm[i], 1000, 2000);
    
    LCD_Show_Trim(i, trims[i]);    
  }
  
  LCD_Show_Number(percent[Get_Most_Active_Ch()]);
  LCD_Show_Battery(BATTERY_TX, Get_Battery_Level());
  LCD_Refresh();

  j++;
  if (j == FRAMERATE) {
    j = 0;
    Serial_Print_Array("ADC", adc_raw, 4);
    Serial_Print_Array("PPM", ppm, 4);
    Serial_Print_Array("Perc", percent, 4);
    Serial.print("\n");
  }
  wait_for_delay();
  PPM_Send();
  Check_Inactivity();
}


void Save_to_EEPROM(void)
{
  uint8_t ee_addr = 0;
  EEPROM.put(ee_addr, calib_min);
  ee_addr += sizeof(calib_min);
  EEPROM.put(ee_addr, calib_mid);
  ee_addr += sizeof(calib_mid);
  EEPROM.put(ee_addr, calib_max); 
  Serial.print("Saved to EEPROM. \n");
}

void Load_from_EEPROM(void)
{
  uint8_t ee_addr = 0;
  EEPROM.get(ee_addr, calib_min);
  ee_addr += sizeof(calib_min);
  EEPROM.get(ee_addr, calib_mid);
  ee_addr += sizeof(calib_mid);
  EEPROM.get(ee_addr, calib_max);
  Serial.print("Loaded from EEPROM. \n");
}




uint8_t Get_Most_Active_Ch(void) 
{
  static uint8_t ch = 0xFF;
  uint16_t change, max_change = 0;

  if (ch == 0xFF) {
    ch = THROTTLE;
    return ch;
  }
  
  for (i = 0; i < 4; i++) {
    change = abs(adc_raw[i] - adc_prev[i]);
    if ((change > max_change) && (change > 50)) {
      max_change = change;
      ch = i;
    }
  }
  return ch;
}


void Check_Inactivity(void)
{
#define INACT_THRESHOLD (5*60*1000UL) // 10 min
#define BEEP_INTERVAL  (10*1000UL) // 10 sec
  
  static uint32_t previous_activity_time = 0;
  static uint32_t next_beep_time = 0;
  
  static uint8_t is_inactive_for_long = 0;

  uint8_t is_active_now = 0;
  
  for (i = 0; i < 4; i++) {
    if (abs(adc_prev[i] - adc_raw[i]) > 20) {
      is_active_now = 1;
    }
  }
  if (is_active_now) {
    previous_activity_time = millis();
    is_inactive_for_long = 0;
  } else {
    if ((millis() - previous_activity_time) > INACT_THRESHOLD) {
      if (!is_inactive_for_long) {
        is_inactive_for_long = 1;
      } else {
        if (millis() > next_beep_time) {
          Beep(1);
          next_beep_time = millis() + BEEP_INTERVAL;
        }
      }   
    }
  }
}








void Serial_Print_Array(const char *title, uint16_t *a, uint8_t len) 
{
  char buff[40];
  uint8_t cnt = 0;
  uint8_t i;
  cnt += sprintf(buff, "%6s: ", title);
  for (i = 0; i < len; i++)
    cnt += sprintf(buff + cnt, "%5d", a[i]);
  sprintf(buff + cnt, "\n");
  Serial.print(buff);
}



uint8_t Get_Battery_Level(void)
{
  #define BAT_MIN  ((1150 * 4 * 1024UL * 47) / (47 + 100) / 3300)   // 1.15V
  #define BAT_MAX  ((1550 * 4 * 1024UL * 47) / (47 + 100) / 3300)   // 1.55 V

  int16_t voltage;
  int8_t level;

  voltage = analogRead(BATTERY_PIN);
  level = map(voltage, BAT_MIN, BAT_MAX, 0, 5);
  level = constrain(level, 0, 5);
  return level;
}


void wait_for_delay() 
{
  static uint32_t next_time = 0;
  while (millis() < next_time) {};
  if (next_time == 0) // first
    next_time = millis() + (1000/FRAMERATE);
  else
    next_time += (1000/FRAMERATE);
}




