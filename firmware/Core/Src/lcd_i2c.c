#include "lcd_i2c.h"
#include "main.h"

/* I2C1 duoc CubeMX khai bao va khoi tao trong main.c. */
extern I2C_HandleTypeDef hi2c1;

#define LCD_I2C_ADDRESS        0x27U
#define LCD_I2C_TIMEOUT_MS     100U

/* PCF8574: P0=RS, P1=RW, P2=EN, P3=Backlight, P4=D4, P5=D5, P6=D6, P7=D7 */
#define LCD_RS                 0x01U
#define LCD_EN                 0x04U
#define LCD_BACKLIGHT          0x08U

static void WriteExpander(uint8_t value)
{
    (void)HAL_I2C_Master_Transmit(&hi2c1,
                                  (uint16_t)(LCD_I2C_ADDRESS << 1U),
                                  &value,
                                  1U,
                                  LCD_I2C_TIMEOUT_MS);
}

/* Gui 4 bit du lieu den D4..D7 va tao xung EN. RW luon bang 0 (ghi). */
static void WriteNibble(uint8_t nibble, uint8_t rs)
{
    uint8_t value = (uint8_t)((nibble << 4U) | LCD_BACKLIGHT);

    if (rs != 0U)
    {
        value |= LCD_RS;
    }

    WriteExpander((uint8_t)(value | LCD_EN));
    WriteExpander(value);
}

/* rs = 0: lenh, rs = 1: du lieu */
static void Send(uint8_t value, uint8_t rs)
{
    WriteNibble((uint8_t)(value >> 4U), rs);
    WriteNibble((uint8_t)(value & 0x0FU), rs);
}

static void SetCursor(uint8_t row)
{
    Send((uint8_t)(0x80U | ((row != 0U) ? 0x40U : 0x00U)), 0U);
}

/* Ghi 16 ky tu cua mot dong */
static void PutLine(uint8_t row, const char *line)
{
    uint8_t i;

    SetCursor(row);
    for (i = 0U; i < 16U; i++)
    {
        Send((uint8_t)line[i], 1U);
    }
}

/* ---- Dung chuoi 16 ky tu ---- */

static uint8_t StrLen(const char *s)
{
    uint8_t n = 0U;
    while (s[n] != '\0') { n++; }
    return n;
}

static void FillSpaces(char *line)
{
    uint8_t i;
    for (i = 0U; i < 16U; i++) { line[i] = ' '; }
    line[16] = '\0';
}

/* Chep s vao line tu cot col */
static void PutText(char *line, uint8_t col, const char *s)
{
    while ((*s != '\0') && (col < 16U))
    {
        line[col] = *s;
        col++;
        s++;
    }
}

/* Doi so thanh chuoi, toi da 5 chu so (lon hon thi hien 99999). */
static uint8_t NumToStr(uint32_t v, char *out)
{
    char    tmp[5];
    uint8_t n = 0U;
    uint8_t i;

    if (v > 99999U) { v = 99999U; }
    if (v == 0U)
    {
        out[0] = '0';
        out[1] = '\0';
        return 1U;
    }

    while (v > 0U)
    {
        tmp[n] = (char)('0' + (v % 10U));
        n++;
        v /= 10U;
    }

    for (i = 0U; i < n; i++)
    {
        out[i] = tmp[n - 1U - i];
    }

    out[n] = '\0';
    return n;
}

/* Tao chuoi "X:123" vao buf */
static void MakeLabel(char *buf, char label, uint32_t v)
{
    buf[0] = label;
    buf[1] = ':';
    (void)NumToStr(v, &buf[2]);
}

void LCD_Init(void)
{
    HAL_Delay(50U);

    /* Dua LCD HD44780 ve che do 4-bit qua PCF8574. */
    WriteNibble(0x03U, 0U); HAL_Delay(5U);
    WriteNibble(0x03U, 0U); HAL_Delay(1U);
    WriteNibble(0x03U, 0U); HAL_Delay(1U);
    WriteNibble(0x02U, 0U); HAL_Delay(1U);

    Send(0x28U, 0U);                 /* 4-bit, 2 dong, 5x8 */
    Send(0x08U, 0U);                 /* tat hien thi */
    Send(0x01U, 0U); HAL_Delay(2U);  /* xoa man hinh */
    Send(0x06U, 0U);                 /* con tro tang */
    Send(0x0CU, 0U);                 /* bat hien thi, tat con tro */
}

void LCD_Update(SystemState_t state,
                SystemMode_t  mode,
                ErrorCode_t   error,
                uint32_t      total,
                uint32_t      countA,
                uint32_t      countB)
{
    char line1[17];
    char line2[17];
    char buf[9];

    FillSpaces(line1);
    FillSpaces(line2);

    if (state == STATE_ERROR)
    {
        PutText(line1, 0U, "STATE: ERROR");

        if (error == ERROR_TIMEOUT)
        {
            PutText(line2, 0U, "ERR: TIMEOUT");
        }
        else if (error == ERROR_CONTINUOUSLY_ACTIVE)
        {
            PutText(line2, 0U, "ERR: CONT_ACTIVE");
        }
        else
        {
            PutText(line2, 0U, "ERR: NONE");
        }
    }
    else
    {
        MakeLabel(buf, 'T', total);
        PutText(line1, 0U, buf);

        MakeLabel(buf, 'B', countB);
        PutText(line1, (uint8_t)(16U - StrLen(buf)), buf);

        MakeLabel(buf, 'A', countA);
        PutText(line2, 0U, buf);

        {
            const char *m = (mode == MODE_AUTO) ? "AUTO" : "MANUAL";
            PutText(line2, (uint8_t)(16U - StrLen(m)), m);
        }
    }

    PutLine(0U, line1);
    PutLine(1U, line2);
}