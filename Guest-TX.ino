
#include "types.h"
#include "LCD.h"
#include "PPM.h"
#include "EEPROM.h"

#define FRAMERATE 45

#define PITCH_PIN     A0
#define ROLL_PIN      A1
#define THROTTLE_PIN  A2
#define YAW_PIN       A3
#define BATTERY_PIN   A4
#define BUTTON_0_PIN  A5
#define BUTTON_1_PIN  A6
#define BEEPER_PIN    3

#define DEBUG_PRINT

uint8_t adcInputs[4] = {ROLL_PIN, PITCH_PIN, THROTTLE_PIN, YAW_PIN};
uint8_t axisInverted[4] = {false, true, false, true};

int16_t adcCurrent[4];
int16_t adcPrevious[4];

int16_t calibMin[4];
int16_t calibMid[4];
int16_t calibMax[4];

int8_t  trims[4];
int16_t percent[4];

void hardwareInit(void)
{
  Serial.begin(9600);
  pinMode(BUTTON_0_PIN, INPUT);
  pinMode(BUTTON_1_PIN, INPUT);
}

uint8_t buttonPressed(uint8_t n)
{
  if (n == 0)
    return !(digitalRead(BUTTON_0_PIN));
  else
    return !(digitalRead(BUTTON_1_PIN));
}

void beep(uint8_t n)
{
  for (uint8_t i = 0; i < n; i++) {
    analogWrite(BEEPER_PIN, 100);
    delay(200);
    analogWrite(BEEPER_PIN, 0);
    delay(200);    
  }
}

uint16_t adcConversion(uint8_t ch) {
  if (axisInverted[ch])
    return (1023 - analogRead(adcInputs[ch]));
  else 
    return (analogRead(adcInputs[ch]));
}

void setup()
{
  hardwareInit();
  ppmInit();
  LCD_init();
  if (buttonPressed(0)) { // enter calibration routine
    LCD_showCalibration();
    LCD_refresh();
    beep(3);
    while (buttonPressed(0)) delay(100);
    while (!buttonPressed(0)) { // user should keep sticks in the center
      static uint8_t b = 0;
      LCD_showAxes(!b);
      for (uint8_t i = 0; i < 4; i++) // blink central marks
        LCD_showTrim(i, LCD_TRIM_CLEAR * b);
      b ^= 1;
      delay(200);  
      LCD_refresh();
    }
    while (buttonPressed(0)) delay(100);
    for (uint8_t i = 0; i < 4; i++) {
      calibMid[i] = adcConversion(i);
      calibMax[i] = calibMid[i];
      calibMin[i] = calibMid[i];
    }
    serialPrintArray("MID", calibMid, 4);    
    beep(3);
    LCD_showAxes(1);
    while (!buttonPressed(0)) { // user should move sticks
      uint16_t adcValue;
      static uint8_t t = 0;
      int8_t s;
      s = (t + (16 - 2 * t) * (t > 8)) - 4;
      for (uint8_t i = 0; i < 4; i++) { // scrolling trim marks
        if (axisInverted[i]) s = -s;
        LCD_showTrim(i, s);
      }
      t++;
      if (t == 16) t = 0;
      LCD_refresh();
      delay(100); 
      for (uint8_t i = 0; i < 4; i++) {
        adcValue = adcConversion(i);
        if (adcValue > calibMax[i])
          calibMax[i] = adcValue;
        if (adcValue < calibMin[i])
          calibMin[i] = adcValue;
      }
    }
    while (buttonPressed(0)) delay(100);
    for (uint8_t i = 0; i < 4; i++) { // keep 10% margin
      calibMin[i] = calibMin[i] + (calibMid[i] - calibMin[i]) / 10;
      calibMax[i] = calibMax[i] - (calibMax[i] - calibMid[i]) / 10;
    }
    serialPrintArray("MIN", calibMin, 4);
    serialPrintArray("MAX", calibMax, 4);
    saveToEeprom();
    beep(3);
  } else {
    beep(1);
  }
  loadFromEeprom();
  serialPrintArray("MIN", calibMin, 4);
  serialPrintArray("MID", calibMid, 4);
  serialPrintArray("MAX", calibMax, 4);
  Serial.print("Starting... \n");
  LCD_showAxes(1);
}

void loop()
{
  static uint8_t frameCnt = 0;
  for (uint8_t i = 0; i < 4; i++) {
    adcPrevious[i] = adcCurrent[i];
    adcCurrent[i] = adcConversion(i);
    if (adcCurrent[i] < calibMid[i]) {
      percent[i] = map(adcCurrent[i] + 5, calibMin[i], calibMid[i],    0,   50);
      ppm[i]     = map(adcCurrent[i], calibMin[i], calibMid[i], 1000, 1500);
    } else {
      percent[i] = map(adcCurrent[i] + 5, calibMid[i], calibMax[i],   50,  100); 
      ppm[i]     = map(adcCurrent[i], calibMid[i], calibMax[i], 1500, 2000);
    } // 1024adc / 100% / 2 = 5 (offset to compensate rounding error)
    trims[i] = map(percent[i], 0, 100, -4, 4);
    trims[i] = constrain(trims[i], -4, 4);
    percent[i] = constrain(percent[i], 0, 100);
    ppm[i] = constrain(ppm[i], 1000, 2000);
    LCD_showTrim(i, trims[i]);    
  }
  LCD_showNumber(percent[getMostActiveChannel()]);
  LCD_showBattery(getBatteryLevel());
  LCD_refresh();
#ifdef DEBUG_PRINT
  frameCnt++;
  if (frameCnt == FRAMERATE) {
    frameCnt = 0;
    serialPrintArray("ADC", adcCurrent, 4);
    serialPrintArray("PPM", ppm, 4);
    serialPrintArray("Perc", percent, 4);
    Serial.print("\n");
  }
#endif
  waitDelay();
  ppmSend();
  checkInactivity();
}

void saveToEeprom(void)
{
  uint8_t eeAddr = 0;
  EEPROM.put(eeAddr, calibMin);
  eeAddr += sizeof(calibMin);
  EEPROM.put(eeAddr, calibMid);
  eeAddr += sizeof(calibMid);
  EEPROM.put(eeAddr, calibMax); 
  Serial.print("Saved to EEPROM. \n");
}

void loadFromEeprom(void)
{
  uint8_t eeAddr = 0;
  EEPROM.get(eeAddr, calibMin);
  eeAddr += sizeof(calibMin);
  EEPROM.get(eeAddr, calibMid);
  eeAddr += sizeof(calibMid);
  EEPROM.get(eeAddr, calibMax);
  Serial.print("Loaded from EEPROM. \n");
}

uint8_t getMostActiveChannel(void) 
{
  static uint8_t ch = 0xFF;
  uint16_t change, maxChange = 0;
  if (ch == 0xFF) { // first call
    ch = THROTTLE;
    return ch;
  }
  for (uint8_t i = 0; i < 4; i++) {
    change = abs(adcCurrent[i] - adcPrevious[i]);
    if ((change > maxChange) && (change > 50)) {
      maxChange = change;
      ch = i;
    }
  }
  return ch;
}

#define INACTIVITY_THRESHOLD       5  // min
#define INACTIVITY_BEEP_INTERVAL  10  // sec

void checkInactivity(void)
{  
  static uint32_t previousActivityTime = 0;
  static uint32_t nextBeepTime = 0;
  static uint8_t isInactiveForLong = 0;
  uint8_t isActiveNow = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (abs(adcPrevious[i] - adcCurrent[i]) > 20) {
      isActiveNow = 1;
    }
  }
  if (isActiveNow) {
    previousActivityTime = millis();
    isInactiveForLong = 0;
  } else {
    if ((millis() - previousActivityTime) > (INACTIVITY_THRESHOLD * 60 * 1000UL)) {
      if (!isInactiveForLong) {
        isInactiveForLong = 1;
      } else {
        if (millis() > nextBeepTime) {
          beep(1);
          nextBeepTime = millis() + (INACTIVITY_BEEP_INTERVAL * 1000UL);
        }
      }   
    }
  }
}

void serialPrintArray(const char *title, uint16_t *a, uint8_t len) 
{
  char buff[40];
  uint8_t cnt = 0;
  uint8_t i;
  cnt += sprintf(buff, "%6s: ", title);
  for (uint8_t i = 0; i < len; i++)
    cnt += sprintf(buff + cnt, "%5d", a[i]);
  sprintf(buff + cnt, "\n");
  Serial.print(buff);
}

// 4xAA batteries, divider 100k-47k, 3.3V power supply  
#define BAT_MIN  ((1150 * 4 * 1024UL * 47) / (47 + 100) / 3300)   // 1.15V
#define BAT_MAX  ((1550 * 4 * 1024UL * 47) / (47 + 100) / 3300)   // 1.55 V

uint8_t getBatteryLevel(void)
{
  int16_t level;
  level = analogRead(BATTERY_PIN);
  level = map(level, BAT_MIN, BAT_MAX, 0, 5);
  level = constrain(level, 0, 5);
  return (uint8_t)level;
}

void waitDelay() 
{
  static uint32_t nextTime = 0;
  while (millis() < nextTime) {};
  if (nextTime == 0) // first call
    nextTime = millis() + (1000 / FRAMERATE);
  else
    nextTime += (1000 / FRAMERATE);
}
