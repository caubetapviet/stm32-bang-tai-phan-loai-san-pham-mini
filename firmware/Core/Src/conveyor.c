#include "conveyor.h"
#include "main.h"
#include "tim.h"

#define CONVEYOR_SPEED_PERCENT  10U   /* chinh khi thu thuc te */

static uint8_t is_running = 0U;

static void SetDuty(uint8_t percent)
{
    uint32_t ccr = ((uint32_t)percent *
                    (__HAL_TIM_GET_AUTORELOAD(&htim4) + 1U)) / 100U;
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, ccr);
}

void Conveyor_Init(void)
{
    HAL_GPIO_WritePin(IN1_L298_GPIO_Port, IN1_L298_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN2_L298_GPIO_Port, IN2_L298_Pin, GPIO_PIN_RESET);
    SetDuty(0U);
    is_running = 0U;
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
}

void Conveyor_Start(void)
{
    HAL_GPIO_WritePin(IN1_L298_GPIO_Port, IN1_L298_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IN2_L298_GPIO_Port, IN2_L298_Pin, GPIO_PIN_RESET);
    SetDuty(CONVEYOR_SPEED_PERCENT);
    is_running = 1U;
}

void Conveyor_Stop(void)
{
    HAL_GPIO_WritePin(IN1_L298_GPIO_Port, IN1_L298_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN2_L298_GPIO_Port, IN2_L298_Pin, GPIO_PIN_RESET);
    SetDuty(0U);
    is_running = 0U;
}

void Conveyor_Toggle(void)
{
    if (is_running != 0U) { Conveyor_Stop(); }
    else                  { Conveyor_Start(); }
}

uint8_t Conveyor_IsRunning(void)
{
    return is_running;
}