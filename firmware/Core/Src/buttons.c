#include "buttons.h"
#include "timer.h"

#define BUTTON_COUNT        6U
#define BUTTON_DEBOUNCE_MS  20U
#define BUTTON_NONE_INDEX   0xFFU   /* khong co nut nao */

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

static uint8_t  inputLocked    = 0U;
static uint8_t  candidate      = BUTTON_NONE_INDEX;  /* nut dang cho debounce */
static uint32_t candidateStart = 0U;                 /* moc bat dau debounce */

static uint8_t ReadButton(GPIO_TypeDef *port, uint16_t pin)
{
    return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET)
           ? 1U : 0U;
}

/* Tra ve chi so nut dau tien dang duoc nhan, hoac BUTTON_NONE_INDEX */
static uint8_t FindPressed(void)
{
    for (uint8_t i = 0U; i < BUTTON_COUNT; i++)
    {
        if (ReadButton(buttons[i].port, buttons[i].pin) != 0U)
        {
            return i;
        }
    }
    return BUTTON_NONE_INDEX;
}

void Buttons_Init(void)
{
    candidate = BUTTON_NONE_INDEX;

    /* Neu dang giu bat ky nut nao luc khoi dong,
       khoa cho den khi tat ca nut duoc nha. */
    inputLocked = (FindPressed() != BUTTON_NONE_INDEX) ? 1U : 0U;
}

ButtonEvent_t Buttons_Update(void)
{
    uint8_t pressed = FindPressed();

    /* Dang khoa: chi mo khoa khi tat ca nut da nha. */
    if (inputLocked != 0U)
    {
        if (pressed == BUTTON_NONE_INDEX)
        {
            inputLocked = 0U;
            candidate = BUTTON_NONE_INDEX;
        }
        return BUTTON_EVENT_NONE;
    }

    /* Khong co nut nao nhan: huy debounce dang cho. */
    if (pressed == BUTTON_NONE_INDEX)
    {
        candidate = BUTTON_NONE_INDEX;
        return BUTTON_EVENT_NONE;
    }

    /* Nut moi (hoac doi nut): bat dau dem debounce. */
    if (pressed != candidate)
    {
        candidate = pressed;
        candidateStart = Timer_GetTick();
        return BUTTON_EVENT_NONE;
    }

    /* Cung mot nut giu on dinh du 20 ms: phat su kien va khoa. */
    if (Timer_IsElapsed(candidateStart, BUTTON_DEBOUNCE_MS) != 0U)
    {
        inputLocked = 1U;
        candidate = BUTTON_NONE_INDEX;
        return buttons[pressed].event;
    }

    return BUTTON_EVENT_NONE;
}
