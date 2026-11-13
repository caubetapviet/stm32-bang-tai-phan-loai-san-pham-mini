
#ifndef TIMER_H
#define TIMER_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* Khoi tao module timer */
void Timer_Init(void);

/* Lay moc thoi gian hien tai, don vi ms */
uint32_t Timer_GetTick(void);

/* Kiem tra da troi qua du thoi gian chua */
uint8_t Timer_IsElapsed(uint32_t startTime, uint32_t duration);

#endif
