#ifndef PPM_H
#define PPM_H

#include <arduino.h>
#include "types.h"

#define PPM_CH_NUM        8
#define PPM_DEFAULT       1500
#define PPM_PIN           4           
#define PPM_PULSE_LENGTH  300

extern int16_t ppm[PPM_CH_NUM];

void ppmInit(void);
void ppmSend(void);

#endif
