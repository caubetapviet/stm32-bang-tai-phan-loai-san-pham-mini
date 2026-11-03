# MODULE SERVO

## 1. Overview

Module `servo` điều khiển servo SG90 gạt sản phẩm loại A bằng PWM của TIM2_CH1 (`PWM_SERVO`, PA0):

### LƯU Ý: configurations đang để TIM2_CH1 trong bản này tạo PWM có tần số 20kHz, khác với `configuration` 

- Chu kỳ PWM 20 ms (50 Hz), 1 đếm = 10 µs (Prescaler = 79, Period = 1999).
- Servo có hai vị trí: `Non-gate` (cánh gạt nằm dọc băng tải, sản phẩm đi thẳng) và `Gate` (cánh gạt quét khoảng 45° so với `Non-gate` để đưa sản phẩm A về đúng vị trí).

Module tự xử lý thời gian trễ `T_delay` và thời gian giữ `T_hold` bằng `Timer_GetTick()`, không dùng `HAL_Delay()`.

Module `servo` không biết trạng thái hệ thống. `main` quyết định khi nào gạt, khi nào đảo vị trí và khi nào đưa servo về `Non-gate`, dựa trên `status`, `sensor` và nút nhấn.

## 2. Responsibilities

- Khởi tạo servo ở vị trí `Non-gate` và khởi động PWM của TIM2.
- Thực hiện một lượt gạt tự động (AUTO): chờ `T_delay`, chuyển sang `Gate`, giữ `T_hold`, trở về `Non-gate`.
- Đảo vị trí `Non-gate` / `Gate` và giữ nguyên vị trí (dùng cho `BTN_SERVO` ở `MODE_MANUAL`).
- Đưa servo về `Non-gate` ngay lập tức và hủy lượt gạt đang chờ.
- Cung cấp vị trí hiện tại cho `main`.

Module không chịu trách nhiệm:

- Quyết định gạt hay không theo `STATE_*`, `MODE_*`, loại sản phẩm.
- Kiểm tra điều kiện cho phép dùng `BTN_SERVO` (do `main` kiểm tra).
- Đọc nút nhấn, đọc cảm biến, nhận diện sản phẩm, cập nhật bộ đếm.
- Bảo đảm khoảng cách đặt sản phẩm (xem mục 5.6).

## 3. Input / Output

### 3.1. Input

| Input | Mô tả |
|---|---|
| `Servo_StartSort()` | Yêu cầu một lượt gạt tự động (`main` gọi khi nhận `SENSOR_EVENT_PRODUCT_A` ở `MODE_AUTO`) |
| `Servo_Toggle()` | Đảo vị trí servo (`main` gọi khi nhấn `BTN_SERVO` ở `MODE_MANUAL`, `STATE_RUNNING`) |
| `Servo_Reset()` | Về `Non-gate` ngay, hủy lượt đang chờ |
| `Servo_Update()` | `main` gọi mỗi vòng lặp để xử lý `T_delay` và `T_hold` |
| `Timer_GetTick()` / `Timer_IsElapsed()` | Lấy thời gian từ module `timer` |

### 3.2. Output

| Output | Mô tả |
|---|---|
| `PWM_SERVO` (PA0, TIM2_CH1) | PWM điều khiển servo, 50 Hz |
| `Servo_IsGate()` | `1U`: đang ở `Gate`, `0U`: đang ở `Non-gate` |

## 4. API Design

### 4.1. Danh sách API

```c
void    Servo_Init(void);
void    Servo_Update(void);
void    Servo_StartSort(void);
void    Servo_Toggle(void);
void    Servo_Reset(void);
uint8_t Servo_IsGate(void);
```

### 4.2. Chức năng từng API

| API | Chức năng |
|---|---|
| `Servo_Init()` | Đặt `CCR = SERVO_PULSE_NON_GATE`, `is_gate = 0`, state `SERVO_IDLE`, rồi gọi `HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1)`. Gọi một lần khi khởi động, sau `MX_TIM2_Init()` và `Timer_Init()`. |
| `Servo_Update()` | `main` gọi mỗi vòng lặp. Ở `SERVO_WAIT_DELAY`: hết `T_delay` thì chuyển servo sang `Gate`, ghi mốc thời gian, sang `SERVO_HOLD`. Ở `SERVO_HOLD`: hết `T_hold` thì về `Non-gate`, sang `SERVO_IDLE`. Ở `SERVO_IDLE`: không làm gì. |
| `Servo_StartSort()` | Nếu state là `SERVO_IDLE`: ghi mốc thời gian, sang `SERVO_WAIT_DELAY`. Nếu servo đang bận (`SERVO_WAIT_DELAY` hoặc `SERVO_HOLD`): bỏ qua lệnh, không báo lại cho `main`. |
| `Servo_Toggle()` | Nếu state là `SERVO_IDLE`: đảo `Non-gate` <-> `Gate` và giữ nguyên vị trí. Nếu servo đang bận: bỏ qua. |
| `Servo_Reset()` | Đặt `CCR = SERVO_PULSE_NON_GATE`, `is_gate = 0`, state `SERVO_IDLE`. Hủy lượt gạt đang chờ. |
| `Servo_IsGate()` | Trả về `is_gate`. Chỉ đọc. |

### 4.3. Ví dụ sử dụng trong `main`

```c
Servo_Init();

/* Sự kiện cảm biến: chỉ sản phẩm A ở MODE_AUTO mới gạt */
if ((ev == SENSOR_EVENT_PRODUCT_A) && (Status_GetMode() == MODE_AUTO))
{
    Servo_StartSort();
}
/* Sản phẩm B: không gọi servo, đi thẳng */

/* Nút SERVO: main kiểm tra điều kiện, module không tự kiểm tra */
case BUTTON_EVENT_SERVO:
    if ((Status_GetState() == STATE_RUNNING) &&
        (Status_GetMode()  == MODE_MANUAL))
    {
        Servo_Toggle();
    }
    break;

/* Mỗi vòng lặp */
Servo_Update();

/* Nút START */
if (Status_Start())
{
    Servo_Reset();        /* bắt đầu ở Non-gate */
}

/* Nút STOP */
if (Status_Stop()) { Servo_Reset(); }

/* Khi vừa vào STATE_ERROR */
Servo_Reset();
```

## 5. Internal Design

### 5.1. State machine

```c
typedef enum
{
    SERVO_IDLE = 0,
    SERVO_WAIT_DELAY,
    SERVO_HOLD
} ServoStateMachine_t;
```

| State | Chức năng |
|---|---|
| `SERVO_IDLE` | Không có lượt gạt nào. Servo giữ nguyên vị trí hiện tại |
| `SERVO_WAIT_DELAY` | Đã nhận lệnh gạt, đang chờ `T_delay`, servo vẫn ở `Non-gate` |
| `SERVO_HOLD` | Servo đang ở `Gate`, chờ hết `T_hold` |

Luồng một lượt gạt: `SERVO_IDLE` -> (`Servo_StartSort()`) -> `SERVO_WAIT_DELAY` -> (hết `T_delay`) -> `SERVO_HOLD` -> (hết `T_hold`) -> `SERVO_IDLE`.

### 5.2. Biến nội bộ

| Biến | Kiểu | Chức năng |
|---|---|---|
| `servo_state` | `ServoStateMachine_t` | State hiện tại |
| `is_gate` | `uint8_t` | `1U`: đang ở `Gate`; `0U`: ở `Non-gate`. Là biến `static` của module |
| `phase_start_tick` | `uint32_t` | Mốc bắt đầu của `T_delay` hoặc `T_hold` hiện tại |

### 5.3. Tham số cấu hình

| Tham số | Ý nghĩa | Giá trị tạm |
|---|---|---|
| `SERVO_PULSE_NON_GATE` | Giá trị CCR ứng với `Non-gate` (đếm, 1 đếm = 10 µs) | 60 (0,6 ms, khoảng 9° tính từ 0,5 ms = 0°) |
| `SERVO_PULSE_GATE` | Giá trị CCR ứng với `Gate` | 110 (1,1 ms, quét khoảng 45° so với `Non-gate`). Dùng 100 nếu muốn đúng 45° so với vị trí tuyệt đối 0° |
| `SERVO_T_DELAY_MS` | Thời gian trễ từ lúc nhận diện A đến lúc gạt (`T_delay`) | 300 ms (chưa chốt) |
| `SERVO_T_HOLD_MS` | Thời gian giữ ở `Gate` (`T_hold`) | 500 ms (chưa chốt) |

Khai báo bằng `#define` hoặc `static const` trong `servo.c`. Chọn cách đặt hằng số theo độ rộng xung, không truyền góc quay vào API.

Quy đổi tham khảo cho SG90: 0,5 ms ứng với 0°, 2,5 ms ứng với 180°, khoảng 11,1 µs mỗi độ (khoảng 0,5 ms mỗi 45°). Mỗi con SG90 lệch vài độ nên các giá trị trên chỉ là điểm xuất phát, chỉnh khi thử thực tế. Nên giữ xung trong khoảng 0,6 ms đến 2,4 ms để tránh servo kẹt cơ khí.

Không dùng xung 0 ms cho `Non-gate`: CCR = 0 khiến chân PWM luôn ở mức thấp, servo không còn tín hiệu và thả lỏng, không giữ được vị trí.

### 5.4. Đặt vị trí

```c
__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse);
```

- `pulse` là `SERVO_PULSE_NON_GATE` hoặc `SERVO_PULSE_GATE`.
- Chu kỳ 20 ms do ARR = 1999 quyết định, đổi CCR chỉ đổi độ rộng xung, không đổi chu kỳ.

### 5.5. Quy ước

- `Servo_IsGate()` phản ánh vị trí được lệnh. Trong `SERVO_WAIT_DELAY` giá trị là `0U`, trong `SERVO_HOLD` là `1U`.
- Các hàm an toàn khi gọi lặp lại. `Servo_Reset()` gọi nhiều lần không gây hại.
- `Servo_StartSort()` khi đang bận bị bỏ qua và không báo lại. Sản phẩm A đến lúc servo bận sẽ không được gạt nhưng vẫn được đếm (bộ đếm do `main` cập nhật độc lập với servo).
- Ở `MODE_MANUAL`, servo giữ nguyên vị trí giữa hai lần nhấn `BTN_SERVO`. Khi sang `STATE_STOP` hoặc `STATE_ERROR`, `main` gọi `Servo_Reset()` để luôn về `Non-gate`.
- Khi cấp điện, `Servo_Init()` đặt `Non-gate` ngay, servo có thể giật nhẹ một lần. Không xử lý thêm.
- `Servo_Update()` phải được gọi thường xuyên ở mỗi vòng lặp chính, không bị chặn bởi `HAL_Delay()` hoặc vòng chờ khác.

### 5.6. Giả định cơ khí

- Băng tải chạy từ trái sang phải, cánh gạt nằm phía hạ lưu của trục servo. Ở `Non-gate`, cánh nằm dọc theo băng tải nên sản phẩm B đi thẳng qua.
- Sản phẩm B không chạy `T_delay` và `T_hold`. Module `servo` và `main` không có logic riêng cho B.
- Việc cánh gạt có quệt vào sản phẩm khác hay không do khoảng cách đặt sản phẩm và cơ khí quyết định (vị trí cảm biến, chiều dài cánh, tốc độ băng tải), firmware không biết vị trí sản phẩm trên băng tải.
- Ràng buộc: `T_delay + T_hold` nhỏ hơn khoảng thời gian tối thiểu giữa hai sản phẩm liên tiếp, áp dụng cho mọi cặp (A-A, A-B, B-A). Khi đó:
  - Sản phẩm đi trước đã ra khỏi tầm quét trước khi servo sang `Gate`.
  - Sản phẩm đi sau chỉ tới cánh gạt sau khi servo đã về `Non-gate`.
  - Servo luôn ở `SERVO_IDLE` khi sản phẩm A tiếp theo được nhận diện.
- Nếu thực tế bị quệt: tăng khoảng cách đặt sản phẩm, giảm `T_hold`, hoặc chỉnh `T_delay`. Không đổi code logic.
- Chiều gạt (theo chiều kim đồng hồ, quét xuống dưới) phụ thuộc cách lắp cánh. Nếu gạt ngược chiều thì đổi cách lắp cánh trên trục servo, không đổi code.

### 5.7. Khởi tạo

`Servo_Init()` đặt CCR về `SERVO_PULSE_NON_GATE` trước khi gọi `HAL_TIM_PWM_Start()`, để xung đầu tiên đã đúng vị trí `Non-gate` (CubeMX đặt `Pulse = 0` ban đầu).

## 6. Dependencies

| Thành phần | Quan hệ | Mô tả |
|---|---|---|
| `main` | Gọi API | Ra lệnh gạt, đảo vị trí, reset; gọi `Servo_Update()` mỗi vòng lặp; đọc `Servo_IsGate()` |
| `timer` | Phụ thuộc | `Timer_GetTick()`, `Timer_IsElapsed()` để tính `T_delay`, `T_hold` |
| `tim.h` | Phụ thuộc | Dùng `htim2` |
| STM32 HAL | Phụ thuộc | TIM PWM |
| `status`, `sensor`, `conveyor`, `data` | Không phụ thuộc | `main` kiểm tra điều kiện rồi gọi API của `servo` |

## 7. Testing

### Test 1: Khởi tạo

- Gọi `Servo_Init()`.
- Kiểm tra PWM 50 Hz, xung `SERVO_PULSE_NON_GATE`, `Servo_IsGate() = 0U`, state `SERVO_IDLE`.

### Test 2: Một lượt gạt hoàn chỉnh

- Gọi `Servo_StartSort()`.
- Kiểm tra servo vẫn ở `Non-gate` trong `T_delay`, sang `Gate` sau `T_delay`, giữ `T_hold`, rồi về `Non-gate` và state về `SERVO_IDLE`.

### Test 3: Bỏ qua lệnh khi đang bận

- Gọi `Servo_StartSort()` hai lần liên tiếp khi servo đang `SERVO_WAIT_DELAY`, rồi lần nữa khi đang `SERVO_HOLD`.
- Kiểm tra chỉ có một lượt gạt, thời gian không bị tính lại.

### Test 4: Không chặn chương trình

- Trong lúc servo chờ `T_delay` và `T_hold`, kiểm tra vòng lặp `main` vẫn chạy bình thường (nút nhấn, cảm biến, LCD).

### Test 5: Toggle

- Ở `SERVO_IDLE`, gọi `Servo_Toggle()` nhiều lần.
- Kiểm tra servo đổi `Non-gate` <-> `Gate`, giữ nguyên vị trí giữa các lần gọi, `Servo_IsGate()` đúng.

### Test 6: Toggle khi đang bận

- Gọi `Servo_StartSort()` rồi `Servo_Toggle()` trước khi lượt gạt kết thúc.
- Kiểm tra `Servo_Toggle()` bị bỏ qua.

### Test 7: Reset

- Gọi `Servo_Reset()` khi servo ở `Gate` (do `Toggle`) và khi đang `SERVO_WAIT_DELAY` hoặc `SERVO_HOLD`.
- Kiểm tra servo về `Non-gate` ngay, lượt gạt bị hủy, không tự sang `Gate` sau đó, state `SERVO_IDLE`.

### Test 8: Gọi lặp lại

- Gọi `Servo_Reset()` nhiều lần liên tiếp.
- Kiểm tra không có hành vi bất thường.

### Test 9: Sản phẩm B

- Cho sản phẩm B đi qua ở `MODE_AUTO`.
- Kiểm tra servo không chuyển động.

### Test 10: Thử cơ khí

- Điều chỉnh `SERVO_PULSE_NON_GATE`, `SERVO_PULSE_GATE`, `T_delay`, `T_hold` trên mô hình.
- Kiểm tra A được gạt đúng, B đi thẳng không bị quệt, servo không kêu hoặc kẹt ở `Non-gate`.

## Các thông số và quyết định chưa chốt

| Nội dung | Trạng thái |
|---|---|
| `SERVO_PULSE_NON_GATE`, `SERVO_PULSE_GATE` | Tạm 60 và 110 (hoặc 100 nếu muốn đúng 45° so với 0° tuyệt đối), chỉnh khi thử thực tế |
| `SERVO_T_DELAY_MS`, `SERVO_T_HOLD_MS` | Tạm 300 ms và 500 ms, chỉnh khi thử thực tế; phải thỏa `T_delay + T_hold` nhỏ hơn khoảng cách tối thiểu giữa hai sản phẩm |
| Chiều gạt | Phụ thuộc cách lắp cánh; đổi cách lắp nếu ngược |
| Báo khi `Servo_StartSort()` bị bỏ qua | Không làm ở bản hiện tại (hàm trả `void`) |
| Xung 0 ms cho `Non-gate` | Không dùng, servo mất tín hiệu và thả lỏng |
| Nguồn servo | SG90 nên cấp 5 V từ nguồn có dòng đủ (module buck), nối chung GND với STM32, tránh sụt áp làm reset MCU khi servo chạy |
