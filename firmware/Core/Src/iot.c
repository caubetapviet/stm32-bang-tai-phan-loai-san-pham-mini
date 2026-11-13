#include "iot.h"
#include "timer.h"
#include "usart.h"

#define IOT_SEND_PERIOD_MS  1000U   /* web nen cap nhat ~1 s */
#define IOT_TX_TIMEOUT_MS   20U
#define IOT_BUF_SIZE        96U

static uint32_t last_send_tick = 0U;

static uint16_t AppendStr(char *buf, uint16_t pos, const char *s)
{
    while ((*s != '\0') && (pos < (IOT_BUF_SIZE - 1U)))
    {
        buf[pos++] = *s++;
    }
    return pos;
}

static uint16_t AppendNum(char *buf, uint16_t pos, uint32_t v)
{
    char    tmp[10];
    uint8_t n = 0U;

    if (v == 0U) { tmp[n++] = '0'; }
    while (v > 0U) { tmp[n++] = (char)('0' + (v % 10U)); v /= 10U; }
    while ((n > 0U) && (pos < (IOT_BUF_SIZE - 1U)))
    {
        buf[pos++] = tmp[--n];
    }
    return pos;
}

void IoT_Init(void)
{
    last_send_tick = Timer_GetTick();
}

void IoT_Update(SystemState_t state, SystemMode_t mode, ErrorCode_t error,
                uint32_t total, uint32_t countA, uint32_t countB)
{
    char     buf[IOT_BUF_SIZE];
    uint16_t pos = 0U;
    const char *s;

    if (Timer_IsElapsed(last_send_tick, IOT_SEND_PERIOD_MS) == 0U) { return; }
    last_send_tick = Timer_GetTick();

    s = (state == STATE_RUNNING) ? "RUNNING" :
        (state == STATE_ERROR)   ? "ERROR"   : "STOP";
    pos = AppendStr(buf, pos, "STATE="); pos = AppendStr(buf, pos, s);

    s = (mode == MODE_AUTO) ? "AUTO" : "MANUAL";
    pos = AppendStr(buf, pos, ";MODE="); pos = AppendStr(buf, pos, s);

    s = (error == ERROR_TIMEOUT)             ? "TIMEOUT"     :
        (error == ERROR_CONTINUOUSLY_ACTIVE) ? "CONT_ACTIVE" : "NONE";
    pos = AppendStr(buf, pos, ";ERR="); pos = AppendStr(buf, pos, s);

    pos = AppendStr(buf, pos, ";T="); pos = AppendNum(buf, pos, total);
    pos = AppendStr(buf, pos, ";A="); pos = AppendNum(buf, pos, countA);
    pos = AppendStr(buf, pos, ";B="); pos = AppendNum(buf, pos, countB);
    pos = AppendStr(buf, pos, "\r\n");

    (void)HAL_UART_Transmit(&huart1, (uint8_t *)buf, pos, IOT_TX_TIMEOUT_MS);
}