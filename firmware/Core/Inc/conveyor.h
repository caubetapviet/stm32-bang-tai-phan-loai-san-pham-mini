#ifndef CONVEYOR_H
#define CONVEYOR_H

#include <stdint.h>

/* Goi sau MX_GPIO_Init() va MX_TIM4_Init() */
void    Conveyor_Init(void);
void    Conveyor_Start(void);
void    Conveyor_Stop(void);
void    Conveyor_Toggle(void);
uint8_t Conveyor_IsRunning(void);

#endif