#include "PPM.h"

uint8_t state = 0;
uint8_t curr_ch = 0;

extern uint16_t ppm[PPM_CH_NUM]; 

volatile uint8_t ppm_flag = 0;

void PPM_Init(void) {
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

void PPM_Send(void) {
  ppm_flag = 0;
  TCNT1 = 0;
  TCCR1B |= (1 << CS11);
  while (!ppm_flag) {};
}

ISR(TIMER1_COMPA_vect){
  TCNT1 = 0;
  if (!state) { // rising edge
    digitalWrite(PPM_PIN, 1);
    OCR1A = PPM_PULSE_LENGTH * 2;
    state = 1;
  } else { // falling edge
    digitalWrite(PPM_PIN, 0);
    state = 0;
    if (curr_ch >= PPM_CH_NUM) {
      curr_ch = 0;
      TCCR1B &= ~(1 << CS11);
      ppm_flag = 1;
    } else {
      OCR1A = (ppm[curr_ch] - PPM_PULSE_LENGTH)*2;
      curr_ch++;
    }
  }
}
