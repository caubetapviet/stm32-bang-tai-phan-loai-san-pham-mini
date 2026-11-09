#include "servo.h"
#include "timer.h"
#include "main.h"
#include "tim.h"

#define SERVO_PULSE_NON_GATE  60U     /* 0,6 ms, 1 dem = 10 us */
#define SERVO_PULSE_GATE      110U    /* 1,1 ms */
#define SERVO_T_HOLD_MS       1000U

static uint8_t  is_gate    = 0U;
static uint8_t  holding    = 0U;   /* dang trong luot gat tu dong */
static uint32_t hold_start = 0U;

static void SetPosition(uint8_t gate)
{
    is_gate = gate;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1,
                          (gate != 0U) ? SERVO_PULSE_GATE : SERVO_PULSE_NON_GATE);
}

void Servo_Init(void)
{
    SetPosition(0U);
    holding = 0U;
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

void Servo_StartSort(void)
{
    if (holding != 0U) { return; }     /* dang ban: bo qua */

    SetPosition(1U);
    hold_start = Timer_GetTick();
    holding    = 1U;
}

void Servo_Toggle(void)
{
    if (holding != 0U) { return; }

    SetPosition((is_gate != 0U) ? 0U : 1U);
}

void Servo_Reset(void)
{
    SetPosition(0U);
    holding = 0U;
}

void Servo_Update(void)
{
    if ((holding != 0U) &&
        (Timer_IsElapsed(hold_start, SERVO_T_HOLD_MS) != 0U))
    {
        SetPosition(0U);
        holding = 0U;
    }
}