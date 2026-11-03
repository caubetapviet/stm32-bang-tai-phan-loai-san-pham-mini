# MODULE CONVEYOR

## 1. Overview

Module `conveyor` điều khiển động cơ DC của băng tải thông qua mạch công suất L298N:

- Hướng quay: hai chân `IN1_L298`, `IN2_L298`.
- Tốc độ: PWM trên chân EN của L298N (`PWM_CONVEYOR`, TIM4_CH1).

Module giữ hai thông tin: băng tải đang chạy hay dừng (`is_running`) và tốc độ đã lưu (`speed_percent`).

Module `conveyor` không biết trạng thái hệ thống. `main` quyết định khi nào chạy, dừng hoặc đảo trạng thái, dựa trên `status` và nút nhấn.

## 2. Responsibilities

- Khởi tạo băng tải ở trạng thái dừng và khởi động PWM của TIM4.
- Chạy băng tải theo một chiều với tốc độ đã lưu.
- Dừng băng tải.
- Đảo trạng thái chạy/dừng (dùng cho `BTN_CONVEYOR` ở `MODE_MANUAL`).
- Lưu và áp dụng tốc độ (phần trăm duty PWM).
- Cung cấp trạng thái chạy/dừng cho `main`.

Module không chịu trách nhiệm:

- Quyết định băng tải chạy hay dừng theo `STATE_*`, `MODE_*`.
- Kiểm tra điều kiện cho phép dùng `BTN_CONVEYOR` (do `main` kiểm tra).
- Đọc nút nhấn, đọc cảm biến, tính thời gian.
- Phanh động cơ và đảo chiều trong lúc vận hành.

## 3. Input / Output

### 3.1. Input

| Input | Mô tả |
|---|---|
| `Conveyor_Start()`, `Conveyor_Stop()`, `Conveyor_Toggle()` | Lệnh điều khiển từ `main` |
| `Conveyor_SetSpeed(percent)` | Tốc độ mong muốn, 0 đến 100 (%) |

### 3.2. Output

| Output | Mô tả |
|---|---|
| `IN1_L298` (PB5), `IN2_L298` (PB4) | Điều khiển hướng quay |
| `PWM_CONVEYOR` (PB6, TIM4_CH1) | PWM tốc độ, nối chân EN của L298N |
| `Conveyor_IsRunning()` | `1U`: đang chạy, `0U`: đang dừng |

## 4. API Design

### 4.1. Danh sách API

```c
void    Conveyor_Init(void);
void    Conveyor_Start(void);
void    Conveyor_Stop(void);
void    Conveyor_Toggle(void);
void    Conveyor_SetSpeed(uint8_t percent);
uint8_t Conveyor_IsRunning(void);
```

### 4.2. Chức năng từng API

| API | Chức năng |
|---|---|
| `Conveyor_Init()` | Đặt `IN1 = IN2 = 0`, `CCR = 0`, `is_running = 0`, `speed_percent = CONVEYOR_SPEED_DEFAULT`, rồi gọi `HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1)`. Gọi một lần khi khởi động, sau `MX_GPIO_Init()` và `MX_TIM4_Init()`. |
| `Conveyor_Start()` | Đặt `IN1 = 1`, `IN2 = 0`, `CCR` theo `speed_percent`, `is_running = 1`. |
| `Conveyor_Stop()` | Đặt `IN1 = 0`, `IN2 = 0`, `CCR = 0`, `is_running = 0`. |
| `Conveyor_Toggle()` | Nếu `is_running = 1` thì gọi `Conveyor_Stop()`, ngược lại gọi `Conveyor_Start()`. |
| `Conveyor_SetSpeed()` | Lưu `speed_percent` (giới hạn tối đa 100). Nếu băng tải đang chạy thì áp dụng ngay vào `CCR`. Nếu đang dừng thì chỉ lưu, không tự chạy. |
| `Conveyor_IsRunning()` | Trả về `is_running`. Chỉ đọc. |

### 4.3. Ví dụ sử dụng trong `main`

```c
Conveyor_Init();
Conveyor_SetSpeed(60U);   /* tùy chọn, bỏ qua thì dùng mặc định */

/* Nút START */
if (Status_Start())
{
    if (Status_GetMode() == MODE_AUTO) { Conveyor_Start(); }
    else                               { Conveyor_Stop();  }  /* MANUAL: băng tải dừng */
}

/* Nút STOP */
if (Status_Stop()) { Conveyor_Stop(); }

/* Nút CONVEYOR: main kiểm tra điều kiện, module không tự kiểm tra */
case BUTTON_EVENT_CONVEYOR:
    if ((Status_GetState() == STATE_RUNNING) &&
        (Status_GetMode()  == MODE_MANUAL))
    {
        Conveyor_Toggle();
    }
    break;

/* Mỗi vòng lặp */
Status_Update(Conveyor_IsRunning(), s.cb_thap_active, s.cb_cao_active, classified);

/* Khi vừa vào STATE_ERROR */
Conveyor_Stop();
```

Ví dụ đổi tốc độ:

```c
Conveyor_Start();         /* chạy ở tốc độ đã lưu */
Conveyor_SetSpeed(80U);   /* đang chạy: đổi sang 80% ngay */
Conveyor_Stop();
Conveyor_SetSpeed(50U);   /* đang dừng: chỉ lưu, chưa chạy */
Conveyor_Start();         /* chạy ở 50% */
```

## 5. Internal Design

### 5.1. Biến nội bộ

| Biến | Kiểu | Chức năng |
|---|---|---|
| `is_running` | `uint8_t` | `1U`: băng tải đang được lệnh chạy; `0U`: dừng. Là biến `static` của module, không phải `STATE_RUNNING` của `status`. |
| `speed_percent` | `uint8_t` | Tốc độ đã lưu, 0 đến 100 (%) |

### 5.2. Tham số cấu hình

| Tham số | Ý nghĩa | Giá trị tạm |
|---|---|---|
| `CONVEYOR_SPEED_DEFAULT` | Tốc độ mặc định sau `Conveyor_Init()` (%) | 70 (chỉnh khi thử nghiệm) |

Khai báo bằng `#define` hoặc `static const` trong `conveyor.c`.

### 5.3. Bảng mức chân

| Hành động | IN1 (PB5) | IN2 (PB4) | CCR |
|---|---|---|---|
| `Conveyor_Start()` | 1 | 0 | theo `speed_percent` |
| `Conveyor_Stop()` | 0 | 0 | 0 |

- Chỉ quay một chiều. Muốn đổi chiều thì đổi dây động cơ hoặc đổi hai mức IN1/IN2 trong `Conveyor_Start()`.
- Dừng kiểu thả trôi (không phanh).

### 5.4. Tính CCR

CCR được tính từ giá trị ARR thực tế của TIM4, không hard-code số mức:

```c
ccr = ((uint32_t)percent * (__HAL_TIM_GET_AUTORELOAD(&htim4) + 1U)) / 100U;
__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, ccr);
```

- ARR chính là Counter Period đặt trong CubeMX, nên đổi Period trong CubeMX thì công thức vẫn đúng.
- `percent = 100` cho `CCR = ARR + 1` (lớn hơn ARR), PWM luôn ở mức cao, tương đương duty 100%.
- `percent = 0` cho `CCR = 0`, PWM luôn ở mức thấp.

### 5.5. Quy ước

- Các hàm an toàn khi gọi lặp lại: gọi `Start()` khi đang chạy hoặc `Stop()` khi đã dừng không gây hại.
- `Conveyor_IsRunning()` phản ánh lệnh chạy/dừng, độc lập với tốc độ. Nếu đặt tốc độ 0 khi đang chạy, motor đứng nhưng `IsRunning()` vẫn là `1U`, nên `status` vẫn tính timeout và T2.
- Giá trị `percent` lớn hơn 100 được cắt về 100.

### 5.6. Khởi tạo

`Conveyor_Init()` tắt IN1, IN2 và đặt `CCR = 0` trước khi bật PWM, để băng tải không quay khi cấp điện.

## 6. Dependencies

| Thành phần | Quan hệ | Mô tả |
|---|---|---|
| `main` | Gọi API | Ra lệnh chạy, dừng, đảo trạng thái, đặt tốc độ; đọc `Conveyor_IsRunning()` |
| `tim.h` | Phụ thuộc | Dùng `htim4` |
| `main.h` | Phụ thuộc | Nhãn chân `IN1_L298_*`, `IN2_L298_*` |
| STM32 HAL | Phụ thuộc | GPIO, TIM PWM |
| `status`, `timer`, `sensor` | Không phụ thuộc | `main` truyền `IsRunning()` vào `Status_Update()` |

## 7. Testing

### Test 1: Khởi tạo

- Gọi `Conveyor_Init()`.
- Kiểm tra `IN1 = IN2 = 0`, `CCR = 0`, `Conveyor_IsRunning() = 0U`, băng tải không quay.

### Test 2: Start

- Gọi `Conveyor_Start()`.
- Kiểm tra `IN1 = 1`, `IN2 = 0`, PWM đúng duty `CONVEYOR_SPEED_DEFAULT`, `IsRunning() = 1U`.

### Test 3: Stop

- Đang chạy, gọi `Conveyor_Stop()`.
- Kiểm tra `IN1 = IN2 = 0`, `CCR = 0`, `IsRunning() = 0U`.

### Test 4: Toggle

- Gọi `Conveyor_Toggle()` hai lần liên tiếp.
- Kiểm tra lần 1 chạy, lần 2 dừng.

### Test 5: SetSpeed khi đang chạy

- Đang chạy, gọi `Conveyor_SetSpeed(80U)`.
- Kiểm tra duty đổi ngay, băng tải vẫn chạy.

### Test 6: SetSpeed khi đang dừng

- Đang dừng, gọi `Conveyor_SetSpeed(50U)`.
- Kiểm tra băng tải không tự chạy, `IsRunning() = 0U`.
- Gọi `Conveyor_Start()`, kiểm tra chạy ở 50%.

### Test 7: Giới hạn tốc độ

- Gọi `Conveyor_SetSpeed(150U)`.
- Kiểm tra tốc độ bị cắt về 100%, PWM luôn mức cao.
- Gọi `Conveyor_SetSpeed(0U)` khi đang chạy: motor đứng, `IsRunning()` vẫn `1U`.

### Test 8: Gọi lặp lại

- Gọi `Start()` hai lần liên tiếp, rồi `Stop()` hai lần liên tiếp.
- Kiểm tra không có hành vi bất thường.

## Các thông số và quyết định chưa chốt

| Nội dung | Trạng thái |
|---|---|
| `CONVEYOR_SPEED_DEFAULT` | Tạm 70%, chỉnh khi thử trên thực tế (F130 có thể cần duty tối thiểu mới quay) |
| Tần số PWM TIM4 | Chuyển xuống 20 kHz: đặt Prescaler = 1, Period = 199 (200 mức). Cần sửa trong `.ioc` |
| `STM32CubeMX_config.md` (mục 3 và mục 6) | Đang ghi Prescaler = 0, 40 kHz; sẽ lệch với `.ioc` mới cho đến khi cập nhật. Không ảnh hưởng các module khác (TIM4 độc lập với TIM1 và TIM2) |
| Dừng kiểu thả trôi hay phanh | Chốt thả trôi. Nếu sản phẩm trượt xa sau khi dừng thì xét thêm phanh (`IN1 = IN2 = 1`, EN mức cao) |
| Soft-start | Không làm ở bản hiện tại |
| Chiều quay | Một chiều; đổi dây hoặc đổi mức IN1/IN2 nếu ngược |
