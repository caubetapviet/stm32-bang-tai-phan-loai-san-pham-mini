
#include "timer.h"

/* Bien dem so mili giay */
static volatile uint32_t g_msTicks = 0U;

/*
 * Khoi tao module timer.
 * TIM1 phai duoc cau hinh boi CubeMX va khoi dong
 * ngat truoc khi su dung module.
 */
void Timer_Init(void)
{
    g_msTicks = 0U;
}

/*
 * Tra ve moc thoi gian hien tai.
 * Tren STM32F103, doc bien uint32_t can chinh
 * thong thuong la thao tac nguyen tu.
 */
uint32_t Timer_GetTick(void)
{
    return g_msTicks;
}

/*
 * Kiem tra thoi gian da troi qua.
 * Phep tru uint32_t xu ly duoc truong hop tran bien.
 */
uint8_t Timer_IsElapsed(uint32_t startTime, uint32_t duration)
{
    uint32_t currentTime = Timer_GetTick();

    if ((uint32_t)(currentTime - startTime) >= duration)
    {
        return 1U;
    }

    return 0U;
}

/*
 * Callback duoc HAL goi khi TIM1 phat sinh ngat.
 * Moi ngat update tuong ung 1 ms.
 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        g_msTicks++;
    }
} 
