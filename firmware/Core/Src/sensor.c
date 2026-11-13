#include "sensor.h"
#include "timer.h"

#define SENSOR_T1_MS  2000U

static uint8_t  t1_active = 0U;   /* dang cho het T1 */
static uint8_t  locked    = 0U;   /* da nhan dien, cho ca hai cam bien ve HIGH */
static uint32_t t1_start  = 0U;

/* Cam bien active-low: LOW = phat hien vat */
static uint8_t ReadActive(GPIO_TypeDef *port, uint16_t pin)
{
    return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) ? 1U : 0U;
}

uint8_t Sensor_IsActive(void)
{
    if ((ReadActive(GPIOB, GPIO_PIN_10) != 0U) ||
        (ReadActive(GPIOB, GPIO_PIN_11) != 0U))
    {
        return 1U;
    }
    return 0U;
}

SensorEvent_t Sensor_Update(uint8_t enable)
{
    SensorEvent_t event = SENSOR_EVENT_NONE;
    uint8_t thap = ReadActive(GPIOB, GPIO_PIN_10);
    uint8_t cao  = ReadActive(GPIOB, GPIO_PIN_11);

    /* Khong cho phep, hoac ca hai cam bien HIGH: xoa het, san sang nhan moi */
    if ((enable == 0U) || ((thap == 0U) && (cao == 0U)))
    {
        t1_active = 0U;
        locked    = 0U;
        return SENSOR_EVENT_NONE;
    }

    /* Da nhan dien, cho san pham roi khoi cam bien */
    if (locked != 0U)
    {
        return SENSOR_EVENT_NONE;
    }

    /* Moi thay cam bien LOW: bat dau tinh T1 */
    if (t1_active == 0U)
    {
        t1_active = 1U;
        t1_start  = Timer_GetTick();
        return SENSOR_EVENT_NONE;
    }

    /* Het T1: doc ket qua */
    if (Timer_IsElapsed(t1_start, SENSOR_T1_MS) != 0U)
    {
        if ((thap != 0U) && (cao != 0U))      { event = SENSOR_EVENT_PRODUCT_A; }
        else if (thap != 0U)                  { event = SENSOR_EVENT_PRODUCT_B; }
        /* chi CB_Cao LOW: khong nhan dien */

        locked    = 1U;      /* du ket qua gi cung phai cho nha cam bien */
        t1_active = 0U;
    }

    return event;
}