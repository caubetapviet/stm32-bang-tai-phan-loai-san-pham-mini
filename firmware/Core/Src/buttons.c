
#include "buttons.h"

#define BUTTON_COUNT 6U

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    ButtonEvent_t event;
} Button_t;

static const Button_t buttons[BUTTON_COUNT] =
{
    {GPIOB, GPIO_PIN_12, BUTTON_EVENT_START},
    {GPIOB, GPIO_PIN_13, BUTTON_EVENT_STOP},
    {GPIOB, GPIO_PIN_14, BUTTON_EVENT_MODE},
    {GPIOB, GPIO_PIN_15, BUTTON_EVENT_RESET},
    {GPIOA, GPIO_PIN_12, BUTTON_EVENT_CONVEYOR},
    {GPIOA, GPIO_PIN_11, BUTTON_EVENT_SERVO}
};

static uint8_t inputLocked = 0U;

static uint8_t ReadButton(GPIO_TypeDef *port, uint16_t pin)
{
    return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET)
           ? 1U : 0U;
}

void Buttons_Init(void)
{
    inputLocked = 0U;

    /* Neu dang giu bat ky nut nao luc khoi dong,
       khoa cho den khi tat ca nut duoc nha. */
    for (uint8_t i = 0U; i < BUTTON_COUNT; i++)
    {
        if (ReadButton(buttons[i].port, buttons[i].pin) != 0U)
        {
            inputLocked = 1U;
            break;
        }
    }
}

ButtonEvent_t Buttons_Update(void)
{
    /* Khi khoa: chi cho phep mo khoa neu tat ca nut da nha. */
    if (inputLocked != 0U)
    {
        for (uint8_t i = 0U; i < BUTTON_COUNT; i++)
        {
            if (ReadButton(buttons[i].port, buttons[i].pin) != 0U)
            {
                return BUTTON_EVENT_NONE;
            }
        }

        inputLocked = 0U;
        return BUTTON_EVENT_NONE;
    }

    /* Khi mo khoa: tim nut dau tien dang duoc nhan. */
    for (uint8_t i = 0U; i < BUTTON_COUNT; i++)
    {
        if (ReadButton(buttons[i].port, buttons[i].pin) != 0U)
        {
            inputLocked = 1U;
            return buttons[i].event;
        }
    }

    return BUTTON_EVENT_NONE;
}
