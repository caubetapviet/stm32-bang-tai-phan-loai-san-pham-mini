# MODULE LCD

## 1. Overview

Module `LCD` điều khiển LCD 16x2 (HD44780, LM016L trong Proteus) và tự chọn nội dung hiển thị theo trạng thái hệ thống (mục 6.1 của `system_requirements.md`).

**Lưu ý: module hiện dùng chế độ 4-bit, nối trực tiếp các chân GPIO. Chế độ I2C (PCF8574T) chưa được sử dụng.**

- LCD chạy 4-bit qua `LCD_RS` (PA1), `LCD_E` (PA2), `LCD_D4` đến `LCD_D7` (PA3 đến PA6).
- Chân RW của LCD nối GND (chỉ ghi, không đọc busy flag). Vì vậy module chờ bằng thời gian cố định sau mỗi lệnh.

Module `LCD` không đọc `status` hay `data`. `main` truyền toàn bộ thông tin cần hiển thị qua tham số của `LCD_Update()`. Module chỉ dùng các kiểu dữ liệu của `status.h` (`SystemState_t`, `SystemMode_t`, `ErrorCode_t`), không gọi hàm nào của `status`.

## 2. Responsibilities

- Khởi tạo LCD ở chế độ 4-bit.
- Dựng nội dung hai dòng theo trạng thái hệ thống (`STATE_STOP`, `STATE_RUNNING`, `STATE_ERROR`).
- Ở `STATE_STOP`, luân phiên hai màn hình, mỗi màn 2 giây.
- Chỉ ghi ra LCD những dòng có nội dung thay đổi so với lần trước.

Module không chịu trách nhiệm:

- Đọc trạng thái, chế độ, mã lỗi, bộ đếm (do `main` truyền vào).
- Đọc nút nhấn, đọc cảm biến, điều khiển thiết bị khác.
- Điều khiển đèn nền LCD.
- Đọc busy flag của LCD (RW nối GND).

## 3. Input / Output

### 3.1. Input

| Input | Mô tả |
|---|---|
| `state` (tham số `LCD_Update()`) | Trạng thái hệ thống hiện tại |
| `mode` | Chế độ hiện tại (`MODE_AUTO` hoặc `MODE_MANUAL`) |
| `error` | Mã lỗi hiện tại (chỉ dùng khi `STATE_ERROR`) |
| `total`, `countA`, `countB` | Giá trị bộ đếm từ RAM (module `data`) |
| `Timer_GetTick()` / `Timer_IsElapsed()` | Lấy thời gian từ module `timer` để đổi màn hình ở `STATE_STOP` |

### 3.2. Output

| Output | Mô tả |
|---|---|
| `LCD_RS` (PA1), `LCD_E` (PA2) | Chọn thanh ghi, xung Enable |
| `LCD_D4` đến `LCD_D7` (PA3 đến PA6) | Dữ liệu 4-bit |

## 4. API Design

### 4.1. Danh sách API

```c
void LCD_Init(void);
void LCD_Update(SystemState_t state,
                SystemMode_t  mode,
                ErrorCode_t   error,
                uint32_t      total,
                uint32_t      countA,
                uint32_t      countB);
```

### 4.2. Chức năng từng API

| API | Chức năng |
|---|---|
| `LCD_Init()` | Khởi tạo LCD 4-bit theo trình tự HD44780 (mục 5.5), xóa bộ đệm `shown`, đặt `prev_state` khác `STATE_STOP` để lần `LCD_Update()` đầu tiên luôn hiện màn 1. Gọi một lần khi khởi động, sau `MX_GPIO_Init()`. |
| `LCD_Update()` | `main` gọi mỗi vòng lặp. Dựng hai dòng theo `state`, so với `shown`, dòng nào khác thì ghi lại. Ở `STATE_STOP` tự đổi màn mỗi 2 giây. |

Các hàm mức thấp (ghi nibble, ghi lệnh, ghi ký tự, đặt con trỏ, trễ) là hàm `static` trong `lcd.c`, `main` không gọi trực tiếp.

### 4.3. Ví dụ sử dụng trong `main`

```c
Timer_Init();
Data_Init();
Status_Init();
LCD_Init();

/* Mỗi vòng lặp */
LCD_Update(Status_GetState(),
           Status_GetMode(),
           Status_GetError(),
           Data_GetCountTotal(),
           Data_GetCountA(),
           Data_GetCountB());
```

`main` không cần biết đang ở màn nào hay đã đổi màn chưa. Gọi `LCD_Update()` mỗi vòng lặp là đủ.

## 5. Internal Design

### 5.1. Nội dung hiển thị

Mỗi dòng luôn được đệm khoảng trắng đủ 16 ký tự.

| Trạng thái | Dòng 1 | Dòng 2 |
|---|---|---|
| `STATE_STOP`, màn 1 | `STATE: STOP` | `MODE: AUTO` hoặc `MODE: MANUAL` |
| `STATE_STOP`, màn 2 | `T:120       B:50` | `A:70` |
| `STATE_RUNNING` | `T:120       B:50` | `A:70        AUTO` hoặc `A:70      MANUAL` |
| `STATE_ERROR` | `STATE: ERROR` | `ERR: TIMEOUT` hoặc `ERR: CONT_ACTIVE` |

Quy ước vị trí:

- `T:` bắt đầu ở cột 0 của dòng 1, số căn trái.
- `B:` và số căn phải, kết thúc ở cột 15 của dòng 1.
- `A:` bắt đầu ở cột 0 của dòng 2, số căn trái.
- Chế độ (`AUTO`, `MANUAL`) căn phải, kết thúc ở cột 15 của dòng 2 (chỉ ở `STATE_RUNNING`).
- Mỗi số tối đa 5 chữ số. Giá trị lớn hơn 99999 hiển thị `99999`.
- `ERROR_NONE` ở `STATE_ERROR` (không nên xảy ra) hiển thị `ERR: NONE`.

### 5.2. Biến nội bộ

| Biến | Kiểu | Chức năng |
|---|---|---|
| `shown[2][17]` | `char` | Nội dung hai dòng đang hiển thị (16 ký tự và ký tự kết thúc chuỗi), để so sánh và chỉ ghi dòng thay đổi |
| `prev_state` | `SystemState_t` | Trạng thái của lần gọi `LCD_Update()` trước, để phát hiện lúc vừa vào `STATE_STOP` |
| `stop_screen` | `uint8_t` | `0U`: đang hiện màn 1; `1U`: đang hiện màn 2 (chỉ dùng ở `STATE_STOP`) |
| `stop_tick` | `uint32_t` | Mốc thời gian lúc màn hiện tại bắt đầu hiện, để tính 2 giây |

### 5.3. Logic đổi màn hình ở STATE_STOP

```c
if (prev_state != STATE_STOP)          /* vừa vào STOP, kể cả lúc cấp điện */
{
    stop_screen = 0U;                  /* luôn hiện màn 1 trước */
    stop_tick   = Timer_GetTick();
}
else if (Timer_IsElapsed(stop_tick, LCD_STOP_SCREEN_MS))
{
    stop_screen ^= 1U;                 /* đổi màn 1 <-> màn 2 */
    stop_tick    = Timer_GetTick();
}
prev_state = state;
```

- `LCD_Init()` đặt `prev_state` khác `STATE_STOP` nên lần gọi đầu tiên (lúc cấp điện) luôn vào nhánh "vừa vào STOP" và hiện màn 1.
- `prev_state` được cập nhật ở mọi trạng thái, không chỉ ở `STATE_STOP`.
- Không dùng `HAL_Delay()` để đổi màn.

### 5.4. Cơ chế chỉ ghi khi thay đổi

1. Dựng hai chuỗi mới (đủ 16 ký tự) vào bộ đệm tạm.
2. Với mỗi dòng, so sánh với `shown[dòng]`.
3. Nếu khác: đặt con trỏ về đầu dòng, ghi 16 ký tự, sao chép vào `shown[dòng]`.
4. Nếu giống: bỏ qua.

Nhờ vậy ở `STATE_RUNNING`, LCD chỉ ghi khi bộ đếm hoặc chế độ đổi, không nhấp nháy. Không dùng lệnh Clear trong `LCD_Update()` (lệnh này chậm, khoảng 1,5 ms); việc ghi đè đủ 16 ký tự đã xóa nội dung cũ. `LCD_Init()` xóa `shown` về giá trị không trùng với bất kỳ nội dung hợp lệ nào, để lần `LCD_Update()` đầu tiên ghi cả hai dòng.

Mỗi lần `LCD_Update()` được gọi, nội dung hiển thị được dựng lại. Đổi số sang chữ số bằng hàm tự viết (vài dòng), không dùng `sprintf`, để không phụ thuộc cấu hình MicroLIB của Keil.

### 5.5. Khởi tạo LCD 4-bit (trong `LCD_Init()`)

| Bước | Hành động | Chờ |
|---|---|---|
| 1 | Chờ LCD ổn định sau cấp nguồn | `HAL_Delay(50)` |
| 2 | Gửi nibble `0x3` (RS = 0) | `HAL_Delay(5)` |
| 3 | Gửi nibble `0x3` | `HAL_Delay(1)` |
| 4 | Gửi nibble `0x3` | `HAL_Delay(1)` |
| 5 | Gửi nibble `0x2` (chuyển sang 4-bit) | `HAL_Delay(1)` |
| 6 | Lệnh `0x28` (4-bit, 2 dòng, font 5x8) | trễ µs |
| 7 | Lệnh `0x08` (tắt hiển thị) | trễ µs |
| 8 | Lệnh `0x01` (xóa màn hình) | `HAL_Delay(2)` |
| 9 | Lệnh `0x06` (con trỏ tăng, không dịch màn hình) | trễ µs |
| 10 | Lệnh `0x0C` (bật hiển thị, tắt con trỏ) | trễ µs |

`HAL_Delay()` chỉ được gọi trong `LCD_Init()` ở vòng lặp chính (một lần lúc khởi động), không gọi trong ngắt (xem `STM32CubeMX_config.md` mục 4).

### 5.6. Ghi một byte (4-bit)

1. Đặt `LCD_RS`: 0 cho lệnh, 1 cho dữ liệu.
2. Đưa nibble cao lên `LCD_D4` đến `LCD_D7`, tạo xung `LCD_E` (HIGH rồi LOW).
3. Đưa nibble thấp lên `LCD_D4` đến `LCD_D7`, tạo xung `LCD_E`.
4. Chờ `LCD_DelayUs(LCD_CMD_DELAY_US)`.

Mỗi chân được ghi bằng `HAL_GPIO_WritePin()` riêng cho dễ đọc. Không ghi thẳng thanh ghi `ODR` của GPIOA vì PA0 (`PWM_SERVO`) và PA7 (`LED_RUN`) cùng cổng.

### 5.7. Trễ

```c
static void LCD_DelayUs(uint32_t us)
{
    volatile uint32_t n = us * LCD_LOOPS_PER_US;
    while (n-- > 0U) { }
}
```

- `volatile` bắt buộc, nếu không trình biên dịch có thể xóa vòng lặp.
- Vòng đếm rỗng chặn CPU nhưng mỗi lần chỉ khoảng 100 µs; ghi một dòng 16 ký tự mất khoảng 2 ms, không ảnh hưởng `Servo_Update()` hay T1 của `sensor`.
- `LCD_LOOPS_PER_US` phụ thuộc mức tối ưu hóa của Keil. Đặt dư rồi chỉnh khi thử (chờ dư không sao, chờ thiếu thì LCD hiện sai ký tự).

### 5.8. Tham số cấu hình

| Tham số | Ý nghĩa | Giá trị tạm |
|---|---|---|
| `LCD_STOP_SCREEN_MS` | Thời gian mỗi màn hình ở `STATE_STOP` | 2000 ms |
| `LCD_CMD_DELAY_US` | Thời gian chờ sau mỗi byte ghi | 100 µs (LCD cần khoảng 40 µs) |
| `LCD_LOOPS_PER_US` | Hệ số của vòng đếm rỗng | Khoảng 1, chỉnh khi thử |

Khai báo bằng `#define` hoặc `static const` trong `lcd.c`.

## 6. Dependencies

| Thành phần | Quan hệ | Mô tả |
|---|---|---|
| `main` | Gọi API | Gọi `LCD_Init()` một lần, gọi `LCD_Update()` mỗi vòng lặp và truyền dữ liệu |
| `status.h` | Phụ thuộc kiểu dữ liệu | `SystemState_t`, `SystemMode_t`, `ErrorCode_t`; không gọi hàm của `status` |
| `timer` | Phụ thuộc | `Timer_GetTick()`, `Timer_IsElapsed()` để đổi màn hình ở `STATE_STOP` |
| `main.h` | Phụ thuộc | Nhãn chân `LCD_RS_*`, `LCD_E_*`, `LCD_D4_*` đến `LCD_D7_*` |
| STM32 HAL | Phụ thuộc | `HAL_GPIO_WritePin()`, `HAL_Delay()` (chỉ trong `LCD_Init()`) |
| `data`, `sensor`, `conveyor`, `servo`, `indicator` | Không phụ thuộc | `main` lấy dữ liệu rồi truyền vào `LCD_Update()` |
| `lcd_i2c.h` | Không dùng | Chế độ I2C chưa sử dụng |

## 7. Testing

### Test 1: Khởi tạo

- Gọi `LCD_Init()`.
- Kiểm tra LCD bật, không hiện ký tự rác, con trỏ tắt.

### Test 2: STATE_STOP, màn 1

- Gọi `LCD_Update()` với `STATE_STOP` ngay sau khi cấp điện.
- Kiểm tra hiện `STATE: STOP` và `MODE: AUTO`; đổi `mode` thì dòng 2 đổi theo `MODE: MANUAL`.

### Test 3: STATE_STOP, luân phiên

- Giữ `STATE_STOP`.
- Kiểm tra màn 1 hiện 2 giây, sau đó màn 2 (`T`, `A`, `B`) hiện 2 giây, lặp lại.

### Test 4: Vào STOP luôn hiện màn 1

- Từ `STATE_RUNNING` hoặc `STATE_ERROR` chuyển sang `STATE_STOP` lúc đang tính màn 2.
- Kiểm tra màn 1 luôn hiện trước.

### Test 5: STATE_RUNNING

- Gọi `LCD_Update()` với `STATE_RUNNING`.
- Kiểm tra bố cục: `T` góc trên trái, `B` góc trên phải (căn phải), `A` góc dưới trái, chế độ góc dưới phải (`AUTO` hoặc `MANUAL`).

### Test 6: Cập nhật bộ đếm

- Tăng `countA`, `countB`, `total` khi đang `STATE_RUNNING`.
- Kiểm tra số đổi đúng, không nhấp nháy, chỉ dòng thay đổi được ghi.

### Test 7: STATE_ERROR

- Gọi với `ERROR_TIMEOUT` và `ERROR_CONTINUOUSLY_ACTIVE`.
- Kiểm tra hiện `STATE: ERROR` và `ERR: TIMEOUT` hoặc `ERR: CONT_ACTIVE`.
- Kiểm tra màn hình giữ nguyên cho đến khi trạng thái đổi.

### Test 8: Giá trị lớn

- Truyền bộ đếm có 5 chữ số và lớn hơn 99999.
- Kiểm tra hiện đủ 5 chữ số, giá trị lớn hơn hiện `99999`, không tràn sang cột khác.

### Test 9: Không chặn chương trình

- Kiểm tra `LCD_Update()` không làm vòng lặp `main` bị trễ đáng kể (nút nhấn, cảm biến, `Servo_Update()` vẫn chạy bình thường).

### Test 10: Chỉnh thời gian trễ

- Nếu LCD hiện sai ký tự hoặc thiếu ký tự, tăng `LCD_LOOPS_PER_US` hoặc `LCD_CMD_DELAY_US`.

## Các thông số và quyết định chưa chốt

| Nội dung | Trạng thái |
|---|---|
| Chế độ giao tiếp | Đang dùng 4-bit (PA1 đến PA6). Chế độ I2C chưa sử dụng, chưa mô phỏng được; nếu chuyển thì phải đổi chân `PWM_CONVEYOR` (PB6) và chỉ thay phần ghi mức thấp trong `lcd.c` |
| `LCD_LOOPS_PER_US`, `LCD_CMD_DELAY_US` | Chỉnh khi thử, phụ thuộc mức tối ưu hóa của Keil |
| Chân RW | Nối GND (đã xác nhận cả Proteus và mạch thật) |
| Mã lỗi hiển thị | `TIMEOUT` và `CONT_ACTIVE` |
| Giới hạn số hiển thị | 5 chữ số, lớn hơn thì hiện `99999` |
| Đèn nền LCD | Không điều khiển bằng firmware |
| Tên file | `lcd.h` / `lcd.c`; không đưa `lcd_i2c.h` vào project Keil |
