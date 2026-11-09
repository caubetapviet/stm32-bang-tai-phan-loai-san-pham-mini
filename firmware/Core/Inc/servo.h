#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

/* Goi sau MX_TIM2_Init() va Timer_Init() */
void Servo_Init(void);
void Servo_Update(void);      /* goi moi vong lap, xu ly T_hold */
void Servo_StartSort(void);   /* AUTO: gat ngay, giu T_hold roi ve Non-gate */
void Servo_Toggle(void);      /* MANUAL: doi Non-gate <-> Gate, giu nguyen */
void Servo_Reset(void);       /* ve Non-gate ngay, huy luot gat */

#endif