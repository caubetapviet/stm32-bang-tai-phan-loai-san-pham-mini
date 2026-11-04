#include "sensor.h"
#include "timer.h"

#define SENSOR_T1_MS  50U

typedef enum
{
    SENSOR_STOPPED = 0,
    SENSOR_READY,
    SENSOR_WAIT_T1,
    SENSOR_WAIT_CLEAR
} SensorStateMachine_t;

static SensorStateMachine_t sensorState = SENSOR_STOPPED;
static SensorState_t        lastState   = {0U, 0U};
static uint32_t             t1StartTick = 0U;

/* Cam bien active-low: LOW = phat hien vat */
static uint8_t ReadActive(GPIO_TypeDef *port, uint16_t pin)
{
    return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET) ? 1U : 0U;
}

void Sensor_Init(void)
{
    sensorState = SENSOR_STOPPED;
    lastState.cb_thap_active = 0U;
    lastState.cb_cao_active  = 0U;
    t1StartTick = 0U;
}

void Sensor_Start(void)
{
    /* Xoa luot dang cho, san sang ngay ke ca cam bien dang LOW */
    t1StartTick = 0U;
    sensorState = SENSOR_READY;
}

void Sensor_Stop(void)
{
    t1StartTick = 0U;
    sensorState = SENSOR_STOPPED;
}

SensorEvent_t Sensor_Update(void)
{
    SensorEvent_t event = SENSOR_EVENT_NONE;

    /* Luon doc cam bien de GetState() phan anh muc hien tai
       (status can muc cam bien de phat hien ERROR_CONTINUOUSLY_ACTIVE) */
    lastState.cb_thap_active = ReadActive(GPIOB, GPIO_PIN_10);
    lastState.cb_cao_active  = ReadActive(GPIOB, GPIO_PIN_11);

    switch (sensorState)
    {
        case SENSOR_STOPPED:
            /* Khong nhan dien, khong phat su kien */
            break;

        case SENSOR_READY:
            if ((lastState.cb_thap_active != 0U) ||
                (lastState.cb_cao_active  != 0U))
            {
                t1StartTick = Timer_GetTick();
                sensorState = SENSOR_WAIT_T1;
            }
            break;

        case SENSOR_WAIT_T1:
            if (Timer_IsElapsed(t1StartTick, SENSOR_T1_MS) != 0U)
            {
                if ((lastState.cb_thap_active != 0U) &&
                    (lastState.cb_cao_active  != 0U))
                {
                    event = SENSOR_EVENT_PRODUCT_A;
                }
                else if ((lastState.cb_thap_active != 0U) &&
                         (lastState.cb_cao_active  == 0U))
                {
                    event = SENSOR_EVENT_PRODUCT_B;
                }
                /* HIGH/LOW va HIGH/HIGH: khong nhan dien (event = NONE) */

                /* Du ket qua la gi cung phai cho ca hai cam bien ve HIGH */
                sensorState = SENSOR_WAIT_CLEAR;
            }
            break;

        case SENSOR_WAIT_CLEAR:
            if ((lastState.cb_thap_active == 0U) &&
                (lastState.cb_cao_active  == 0U))
            {
                sensorState = SENSOR_READY;
            }
            break;

        default:
            sensorState = SENSOR_STOPPED;
            break;
    }

    return event;
}

void Sensor_GetState(SensorState_t *state)
{
    if (state != 0)
    {
        *state = lastState;
    }
}
