#include "indicator.h"
#include "main.h"

#define INDICATOR_ON   GPIO_PIN_SET
#define INDICATOR_OFF  GPIO_PIN_RESET

void Indicator_Init(void)
{
    Indicator_SetRun(0U);
    Indicator_SetFault(0U);
}

void Indicator_SetRun(uint8_t on)
{
    HAL_GPIO_WritePin(LED_RUN_GPIO_Port, LED_RUN_Pin,
                      (on != 0U) ? INDICATOR_ON : INDICATOR_OFF);
}

void Indicator_SetFault(uint8_t on)
{
    GPIO_PinState level = (on != 0U) ? INDICATOR_ON : INDICATOR_OFF;

    HAL_GPIO_WritePin(LED_FAULT_GPIO_Port, LED_FAULT_Pin, level);
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, level);
}