#ifndef INDICATOR_H
#define INDICATOR_H

#include <stdint.h>

void Indicator_Init(void);
void Indicator_SetRun(uint8_t on);     /* LED_RUN */
void Indicator_SetFault(uint8_t on);   /* LED_FAULT + BUZZER */

#endif