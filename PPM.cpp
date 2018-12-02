#include "PPM.h"

int16_t ppm[PPM_CH_NUM];
uint8_t ppmOutputState = 0;
uint8_t ppmCurrentChannel = 0;
volatile uint8_t ppmDoneFlag = 0;

void ppmInit(void) {
  for (uint8_t i = 0; i < PPM_CH_NUM; i++)
    ppm[i] = PPM_DEFAULT;
  pinMode(PPM_PIN, OUTPUT);
  digitalWrite(PPM_PIN, 1);
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16MHz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
}

void ppmSend(void) {
  ppmDoneFlag = 0;
  TCNT1 = 0;
  TCCR1B |= (1 << CS11);
  while (!ppmDoneFlag) {};
}

ISR(TIMER1_COMPA_vect){
  TCNT1 = 0;
  if (!ppmOutputState) { // rising edge
    digitalWrite(PPM_PIN, 1);
    OCR1A = PPM_PULSE_LENGTH * 2;
    ppmOutputState = 1;
  } else { // falling edge
    digitalWrite(PPM_PIN, 0);
    ppmOutputState = 0;
    if (ppmCurrentChannel >= PPM_CH_NUM) {
      ppmCurrentChannel = 0;
      TCCR1B &= ~(1 << CS11);
      ppmDoneFlag = 1;
    } else {
      OCR1A = (ppm[ppmCurrentChannel] - PPM_PULSE_LENGTH)*2;
      ppmCurrentChannel++;
    }
  }
}
